/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MTK_CMDQ_H__
#define __MTK_CMDQ_H__

#include <linux/mailbox_client.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>

#define CMDQ_SPR_FOR_TEMP		(0)
#define CMDQ_THR_SPR_IDX0		0
#define CMDQ_THR_SPR_IDX1		1
#define CMDQ_THR_SPR_IDX2		2
#define CMDQ_THR_SPR_IDX3		3
#define CMDQ_THR_SPR_MAX		(4)

#define CMDQ_TPR_ID			(56)
#define CMDQ_CPR_STRAT_ID		(0x8000)

#if IS_ENABLED(CONFIG_MACH_MT6771) || IS_ENABLED(CONFIG_MACH_MT6765) || \
	IS_ENABLED(CONFIG_MACH_MT6761)
#define CMDQ_REG_SHIFT_ADDR(addr)	(addr)
#define CMDQ_REG_REVERT_ADDR(addr)	(addr)
#elif IS_ENABLED(CONFIG_MACH_MT3967)
#define CMDQ_REG_SHIFT_ADDR(addr)	((addr) >> 3)
#define CMDQ_REG_REVERT_ADDR(addr)	((addr) << 3)
#endif

typedef u64 CMDQ_VARIABLE;

/* software token in CMDQ */
enum cmdq_event {
	/* SW Sync Tokens (Pre-defined) */
	/* Config thread notify trigger thread */
	CMDQ_SYNC_TOKEN_CONFIG_DIRTY = 401,
	/* Trigger thread notify config thread */
	CMDQ_SYNC_TOKEN_STREAM_EOF = 402,
	/* Block Trigger thread until the ESD check finishes. */
	CMDQ_SYNC_TOKEN_ESD_EOF = 403,
	/* check CABC setup finish */
	CMDQ_SYNC_TOKEN_CABC_EOF = 404,
	/* Block Trigger thread until the path freeze finishes */
	CMDQ_SYNC_TOKEN_FREEZE_EOF = 405,
	/* Pass-2 notifies VENC frame is ready to be encoded */
	CMDQ_SYNC_TOKEN_VENC_INPUT_READY = 406,
	/* VENC notifies Pass-2 encode done so next frame may start */
	CMDQ_SYNC_TOKEN_VENC_EOF = 407,

	/* Notify normal CMDQ there are some secure task done */
	CMDQ_SYNC_SECURE_THR_EOF = 408,
	/* Lock WSM resource */
	CMDQ_SYNC_SECURE_WSM_LOCK = 409,

	/* SW Sync Tokens (User-defined) */
	CMDQ_SYNC_TOKEN_USER_0 = 410,
	CMDQ_SYNC_TOKEN_USER_1 = 411,
	CMDQ_SYNC_TOKEN_POLL_MONITOR = 412,

	/* SW Sync Tokens (Pre-defined) */
	/* Config thread notify trigger thread for external display */
	CMDQ_SYNC_TOKEN_EXT_CONFIG_DIRTY = 415,
	/* Trigger thread notify config thread */
	CMDQ_SYNC_TOKEN_EXT_STREAM_EOF = 416,
	/* Check CABC setup finish */
	CMDQ_SYNC_TOKEN_EXT_CABC_EOF = 417,

	/* Secure video path notify SW token */
	CMDQ_SYNC_DISP_OVL0_2NONSEC_END = 420,
	CMDQ_SYNC_DISP_OVL1_2NONSEC_END = 421,
	CMDQ_SYNC_DISP_2LOVL0_2NONSEC_END = 422,
	CMDQ_SYNC_DISP_2LOVL1_2NONSEC_END = 423,
	CMDQ_SYNC_DISP_RDMA0_2NONSEC_END = 424,
	CMDQ_SYNC_DISP_RDMA1_2NONSEC_END = 425,
	CMDQ_SYNC_DISP_WDMA0_2NONSEC_END = 426,
	CMDQ_SYNC_DISP_WDMA1_2NONSEC_END = 427,
	CMDQ_SYNC_DISP_EXT_STREAM_EOF = 428,

	/**
	 * Event for CMDQ to block executing command when append command
	 * Plz sync CMDQ_SYNC_TOKEN_APPEND_THR(id) in cmdq_core source file.
	 */
	CMDQ_SYNC_TOKEN_APPEND_THR0 = 432,
	CMDQ_SYNC_TOKEN_APPEND_THR1 = 433,
	CMDQ_SYNC_TOKEN_APPEND_THR2 = 434,
	CMDQ_SYNC_TOKEN_APPEND_THR3 = 435,
	CMDQ_SYNC_TOKEN_APPEND_THR4 = 436,
	CMDQ_SYNC_TOKEN_APPEND_THR5 = 437,
	CMDQ_SYNC_TOKEN_APPEND_THR6 = 438,
	CMDQ_SYNC_TOKEN_APPEND_THR7 = 439,
	CMDQ_SYNC_TOKEN_APPEND_THR8 = 440,
	CMDQ_SYNC_TOKEN_APPEND_THR9 = 441,
	CMDQ_SYNC_TOKEN_APPEND_THR10 = 442,
	CMDQ_SYNC_TOKEN_APPEND_THR11 = 443,
	CMDQ_SYNC_TOKEN_APPEND_THR12 = 444,
	CMDQ_SYNC_TOKEN_APPEND_THR13 = 445,
	CMDQ_SYNC_TOKEN_APPEND_THR14 = 446,
	CMDQ_SYNC_TOKEN_APPEND_THR15 = 447,
	CMDQ_SYNC_TOKEN_APPEND_THR16 = 448,
	CMDQ_SYNC_TOKEN_APPEND_THR17 = 449,
	CMDQ_SYNC_TOKEN_APPEND_THR18 = 450,
	CMDQ_SYNC_TOKEN_APPEND_THR19 = 451,
	CMDQ_SYNC_TOKEN_APPEND_THR20 = 452,
	CMDQ_SYNC_TOKEN_APPEND_THR21 = 453,
	CMDQ_SYNC_TOKEN_APPEND_THR22 = 454,
	CMDQ_SYNC_TOKEN_APPEND_THR23 = 455,
	CMDQ_SYNC_TOKEN_APPEND_THR24 = 456,
	CMDQ_SYNC_TOKEN_APPEND_THR25 = 457,
	CMDQ_SYNC_TOKEN_APPEND_THR26 = 458,
	CMDQ_SYNC_TOKEN_APPEND_THR27 = 459,
	CMDQ_SYNC_TOKEN_APPEND_THR28 = 460,
	CMDQ_SYNC_TOKEN_APPEND_THR29 = 461,
	CMDQ_SYNC_TOKEN_APPEND_THR30 = 462,
	CMDQ_SYNC_TOKEN_APPEND_THR31 = 463,

	/* GPR access tokens (for HW register backup)
	 * There are 15 32-bit GPR, 3 GPR form a set
	 * (64-bit for address, 32-bit for value)
	 */
	CMDQ_SYNC_TOKEN_GPR_SET_0 = 470,
	CMDQ_SYNC_TOKEN_GPR_SET_1 = 471,
	CMDQ_SYNC_TOKEN_GPR_SET_2 = 472,
	CMDQ_SYNC_TOKEN_GPR_SET_3 = 473,
	CMDQ_SYNC_TOKEN_GPR_SET_4 = 474,

	/* Resource lock event to control resource in GCE thread */
	CMDQ_SYNC_RESOURCE_WROT0 = 480,
	CMDQ_SYNC_RESOURCE_WROT1 = 481,

	/* Event for CMDQ delay implement
	 * Plz sync CMDQ_SYNC_TOKEN_DELAY_THR(id) in cmdq_core source file.
	 */
	CMDQ_SYNC_TOKEN_TIMER = 485,
	CMDQ_SYNC_TOKEN_DELAY_SET0 = 486,
	CMDQ_SYNC_TOKEN_DELAY_SET1 = 487,
	CMDQ_SYNC_TOKEN_DELAY_SET2 = 488,

	/* GCE HW TPR Event*/
	CMDQ_EVENT_TIMER_00 = 962,
	CMDQ_EVENT_TIMER_01 = 963,
	CMDQ_EVENT_TIMER_02 = 964,
	CMDQ_EVENT_TIMER_03 = 965,
	CMDQ_EVENT_TIMER_04 = 966,
	/* 5: 1us */
	CMDQ_EVENT_TIMER_05 = 967,
	CMDQ_EVENT_TIMER_06 = 968,
	CMDQ_EVENT_TIMER_07 = 969,
	/* 8: 10us */
	CMDQ_EVENT_TIMER_08 = 970,
	CMDQ_EVENT_TIMER_09 = 971,
	CMDQ_EVENT_TIMER_10 = 972,
	/* 11: 100us */
	CMDQ_EVENT_TIMER_11 = 973,
	CMDQ_EVENT_TIMER_12 = 974,
	CMDQ_EVENT_TIMER_13 = 975,
	CMDQ_EVENT_TIMER_14 = 976,
	/* 15: 1ms */
	CMDQ_EVENT_TIMER_15 = 977,
	CMDQ_EVENT_TIMER_16 = 978,
	CMDQ_EVENT_TIMER_17 = 979,
	/* 18: 10ms */
	CMDQ_EVENT_TIMER_18 = 980,
	CMDQ_EVENT_TIMER_19 = 981,
	CMDQ_EVENT_TIMER_20 = 982,
	/* 21: 100ms */
	CMDQ_EVENT_TIMER_21 = 983,
	CMDQ_EVENT_TIMER_22 = 984,
	CMDQ_EVENT_TIMER_23 = 985,
	CMDQ_EVENT_TIMER_24 = 986,
	CMDQ_EVENT_TIMER_25 = 987,
	CMDQ_EVENT_TIMER_26 = 988,
	CMDQ_EVENT_TIMER_27 = 989,
	CMDQ_EVENT_TIMER_28 = 990,
	CMDQ_EVENT_TIMER_29 = 991,
	CMDQ_EVENT_TIMER_30 = 992,
	CMDQ_EVENT_TIMER_31 = 993,

	/* event id is 9 bit */
	CMDQ_SYNC_TOKEN_MAX = 0x3FF,
	CMDQ_SYNC_TOKEN_INVALID = -1,
};

/* General Purpose Register */
enum cmdq_gpr_reg {
	/* Value Reg, we use 32-bit */
	/* Address Reg, we use 64-bit */
	/* Note that R0-R15 and P0-P7 actullay share same memory */
	/* and R1 cannot be used. */

	CMDQ_DATA_REG_JPEG = 0x00,	/* R0 */
	CMDQ_DATA_REG_JPEG_DST = 0x11,	/* P1 */

	CMDQ_DATA_REG_PQ_COLOR = 0x04,	/* R4 */
	CMDQ_DATA_REG_PQ_COLOR_DST = 0x13,	/* P3 */

	CMDQ_DATA_REG_2D_SHARPNESS_0 = 0x05,	/* R5 */
	CMDQ_DATA_REG_2D_SHARPNESS_0_DST = 0x14,	/* P4 */

	CMDQ_DATA_REG_2D_SHARPNESS_1 = 0x0a,	/* R10 */
	CMDQ_DATA_REG_2D_SHARPNESS_1_DST = 0x16,	/* P6 */

	CMDQ_DATA_REG_DEBUG = 0x0b,	/* R11 */
	CMDQ_DATA_REG_DEBUG_DST = 0x17,	/* P7 */

	/* sentinel value for invalid register ID */
	CMDQ_DATA_REG_INVALID = -1,
};

struct cmdq_pkt;

struct cmdq_subsys {
	u32 base;
	u8 id;
};

struct cmdq_base {
	struct cmdq_subsys subsys[32];
	u8 count;
	u16 cpr_base;
	u8 cpr_cnt;
};

struct cmdq_client {
	struct mbox_client client;
	struct mbox_chan *chan;
	void *cl_priv;
};

struct cmdq_operand {
	/* register type */
	bool reg;
	union {
		/* index */
		u16 idx;
		/* value */
		u16 value;
	};
};

enum CMDQ_LOGIC_ENUM {
	CMDQ_LOGIC_ASSIGN = 0,
	CMDQ_LOGIC_ADD = 1,
	CMDQ_LOGIC_SUBTRACT = 2,
	CMDQ_LOGIC_MULTIPLY = 3,
	CMDQ_LOGIC_XOR = 8,
	CMDQ_LOGIC_NOT = 9,
	CMDQ_LOGIC_OR = 10,
	CMDQ_LOGIC_AND = 11,
	CMDQ_LOGIC_LEFT_SHIFT = 12,
	CMDQ_LOGIC_RIGHT_SHIFT = 13
};

enum CMDQ_CONDITION_ENUM {
	CMDQ_CONDITION_ERROR = -1,

	/* these are actual HW op code */
	CMDQ_EQUAL = 0,
	CMDQ_NOT_EQUAL = 1,
	CMDQ_GREATER_THAN_AND_EQUAL = 2,
	CMDQ_LESS_THAN_AND_EQUAL = 3,
	CMDQ_GREATER_THAN = 4,
	CMDQ_LESS_THAN = 5,

	CMDQ_CONDITION_MAX,
};

struct cmdq_flush_completion {
	struct cmdq_pkt *pkt;
	struct completion cmplt;
	s32 err;
};

u32 cmdq_subsys_id_to_base(struct cmdq_base *cmdq_base, int id);

/**
 * cmdq_pkt_realloc_cmd_buffer() - reallocate command buffer for CMDQ packet
 * @pkt:	the CMDQ packet
 * @size:	the request size
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_realloc_cmd_buffer(struct cmdq_pkt *pkt, size_t size);

/**
 * cmdq_register_device() - register device which needs CMDQ
 * @dev:	device for CMDQ to access its registers
 *
 * Return: cmdq_base pointer or NULL for failed
 */
struct cmdq_base *cmdq_register_device(struct device *dev);

/**
 * cmdq_mbox_create() - create CMDQ mailbox client and channel
 * @dev:	device of CMDQ mailbox client
 * @index:	index of CMDQ mailbox channel
 *
 * Return: CMDQ mailbox client pointer
 */
struct cmdq_client *cmdq_mbox_create(struct device *dev, int index);
void cmdq_mbox_stop(struct cmdq_client *cl);

void cmdq_mbox_pool_set_limit(struct cmdq_client *cl, u32 limit);
void cmdq_mbox_pool_create(struct cmdq_client *cl);
void cmdq_mbox_pool_clear(struct cmdq_client *cl);

void *cmdq_mbox_buf_alloc(struct device *dev, dma_addr_t *pa_out);
void cmdq_mbox_buf_free(struct device *dev, void *va, dma_addr_t pa);

s32 cmdq_dev_get_event(struct device *dev, const char *name);

struct cmdq_pkt_buffer *cmdq_pkt_alloc_buf(struct cmdq_pkt *pkt);

void cmdq_pkt_free_buf(struct cmdq_pkt *pkt);

s32 cmdq_pkt_add_cmd_buffer(struct cmdq_pkt *pkt);

/**
 * cmdq_mbox_destroy() - destroy CMDQ mailbox client and channel
 * @client:	the CMDQ mailbox client
 */
void cmdq_mbox_destroy(struct cmdq_client *client);

/**
 * cmdq_pkt_create() - create a CMDQ packet
 * @pkt_ptr:	CMDQ packet pointer to retrieve cmdq_pkt
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_create(struct cmdq_pkt **pkt_ptr);

void cmdq_pkt_set_client(struct cmdq_pkt *pkt, struct cmdq_client *cl);

s32 cmdq_pkt_cl_create(struct cmdq_pkt **pkt_ptr, struct cmdq_client *cl);

/**
 * cmdq_pkt_destroy() - destroy the CMDQ packet
 * @pkt:	the CMDQ packet
 */
void cmdq_pkt_destroy(struct cmdq_pkt *pkt);

u64 *cmdq_pkt_get_va_by_offset(struct cmdq_pkt *pkt, size_t offset);

dma_addr_t cmdq_pkt_get_pa_by_offset(struct cmdq_pkt *pkt, u32 offset);

s32 cmdq_pkt_append_command(struct cmdq_pkt *pkt, u16 arg_c, u16 arg_b,
	u16 arg_a, u8 s_op, u8 arg_c_type, u8 arg_b_type, u8 arg_a_type,
	enum cmdq_code code);

s32 cmdq_pkt_read(struct cmdq_pkt *pkt, struct cmdq_base *clt_base,
	u32 src_addr, u16 dst_reg_idx);

s32 cmdq_pkt_read_reg(struct cmdq_pkt *pkt, u8 subsys, u16 offset,
	u16 dst_reg_idx);

s32 cmdq_pkt_read_addr(struct cmdq_pkt *pkt, u32 addr, u16 dst_reg_idx);

s32 cmdq_pkt_write_reg(struct cmdq_pkt *pkt, u8 subsys,
	u16 offset, u16 src_reg_idx, u32 mask);

s32 cmdq_pkt_write_value(struct cmdq_pkt *pkt, u8 subsys,
	u16 offset, u32 value, u32 mask);

s32 cmdq_pkt_write_reg_addr(struct cmdq_pkt *pkt, u32 addr,
	u16 src_reg_idx, u32 mask);

s32 cmdq_pkt_write_value_addr(struct cmdq_pkt *pkt, u32 addr,
	u32 value, u32 mask);

s32 cmdq_pkt_store_value(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
	u32 value, u32 mask);

s32 cmdq_pkt_store_value_reg(struct cmdq_pkt *pkt, u16 indirect_dst_reg_idx,
	u16 indirect_src_reg_idx, u32 mask);

s32 cmdq_pkt_write_indriect(struct cmdq_pkt *pkt, struct cmdq_base *clt_base,
	u32 addr, u16 src_reg_idx, u32 mask);

/**
 * cmdq_pkt_write() - append write command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @clt_base:	the CMDQ base
 * @addr:	target register address
 * @mask:	the specified target register mask
 *
 * Return: 0 for success; else the error code is returned
 */
s32 cmdq_pkt_write(struct cmdq_pkt *pkt, struct cmdq_base *clt_base,
	u32 addr, u32 value, u32 mask);

s32 cmdq_pkt_mem_move(struct cmdq_pkt *pkt, struct cmdq_base *clt_base,
	u32 src_addr, u32 dst_addr, u16 swap_reg_idx);

s32 cmdq_pkt_assign_command(struct cmdq_pkt *pkt, u16 reg_idx, u32 value);

s32 cmdq_pkt_logic_command(struct cmdq_pkt *pkt, enum CMDQ_LOGIC_ENUM s_op,
	u16 result_reg_idx,
	struct cmdq_operand *left_operand,
	struct cmdq_operand *right_operand);

s32 cmdq_pkt_jump(struct cmdq_pkt *pkt, s32 offset);

s32 cmdq_pkt_jump_addr(struct cmdq_pkt *pkt, u32 addr);

s32 cmdq_pkt_poll_addr(struct cmdq_pkt *pkt, u32 value, u32 addr, u32 mask);

s32 cmdq_pkt_poll_reg(struct cmdq_pkt *pkt, u32 value, u8 subsys,
	u16 offset, u32 mask);

/**
 * cmdq_pkt_poll() - append polling command with mask to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @value:	the specified target register value
 * @subsys:	the CMDQ subsys id
 * @offset:	register offset from module base
 * @mask:	the specified target register mask
 *
 * Return: 0 for success; else the error code is returned
 */
s32 cmdq_pkt_poll(struct cmdq_pkt *pkt, struct cmdq_base *clt_base,
	u32 value, u32 addr, u32 mask);

/**
 * cmdq_pkt_wfe() - append wait for event command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @event:	the desired event type to "wait and CLEAR"
 *
 * Return: 0 for success; else the error code is returned
 */
int cmdq_pkt_wfe(struct cmdq_pkt *pkt, u16 event);

int cmdq_pkt_wait_no_clear(struct cmdq_pkt *pkt, u16 event);

/**
 * cmdq_pkt_clear_event() - append clear event command to the CMDQ packet
 * @pkt:	the CMDQ packet
 * @event:	the desired event to be cleared
 *
 * Return: 0 for success; else the error code is returned
 */
s32 cmdq_pkt_clear_event(struct cmdq_pkt *pkt, u16 event);

s32 cmdq_pkt_set_event(struct cmdq_pkt *pkt, u16 event);

s32 cmdq_pkt_finalize(struct cmdq_pkt *pkt);

s32 cmdq_pkt_finalize_loop(struct cmdq_pkt *pkt);

/**
 * cmdq_pkt_flush_async() - trigger CMDQ to asynchronously execute the CMDQ
 *                          packet and call back at the end of done packet
 * @client:	the CMDQ mailbox client
 * @pkt:	the CMDQ packet
 * @cb:		called at the end of done packet
 * @data:	this data will pass back to cb
 *
 * Return: 0 for success; else the error code is returned
 *
 * Trigger CMDQ to asynchronously execute the CMDQ packet and call back
 * at the end of done packet. Note that this is an ASYNC function. When the
 * function returned, it may or may not be finished.
 */
s32 cmdq_pkt_flush_async(struct cmdq_client *client, struct cmdq_pkt *pkt,
	cmdq_async_flush_cb cb, void *data);

s32 cmdq_pkt_flush_threaded(struct cmdq_client *client, struct cmdq_pkt *pkt,
	cmdq_async_flush_cb cb, void *data);

/**
 * cmdq_pkt_flush() - trigger CMDQ to execute the CMDQ packet
 * @client:	the CMDQ mailbox client
 * @pkt:	the CMDQ packet
 *
 * Return: 0 for success; else the error code is returned
 *
 * Trigger CMDQ to execute the CMDQ packet. Note that this is a
 * synchronous flush function. When the function returned, the recorded
 * commands have been done.
 */
s32 cmdq_pkt_flush(struct cmdq_client *client, struct cmdq_pkt *pkt);

s32 cmdq_pkt_dump_buf(struct cmdq_pkt *pkt, u32 curr_pa);

int cmdq_dump_pkt(struct cmdq_pkt *pkt);


struct cmdq_thread_task_info {
	dma_addr_t		pa_base;
	struct cmdq_pkt		*pkt;
	struct list_head	list_entry;
};

struct cmdq_timeout_info {
	u32 irq;
	u32 irq_en;
	dma_addr_t curr_pc;
	u32 *curr_pc_va;
	dma_addr_t end_addr;
	u32 task_num;
	struct cmdq_thread_task_info *timeout_task;
	struct list_head task_list;
};
#endif	/* __MTK_CMDQ_H__ */
