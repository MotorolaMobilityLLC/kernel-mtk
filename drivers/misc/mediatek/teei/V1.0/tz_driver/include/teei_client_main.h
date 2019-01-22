/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
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

#define TLOG_SIZE       (256 * 1024)

#define MICROTRUST_DRM_SUPPORT

#ifdef MICROTRUST_DRM_SUPPORT

#define TOTAL_DRM_DRIVER_NUM (8)
enum drm_dcih_buf_mode {
	DRM_DCIH_BUF_MODE_FORWARD = 0x0,
	DRM_DCIH_BUF_MODE_BACKWARD = 0x1,

	DRM_DCIH_BUF_MODE_INVALID = 0xFF
};
struct drm_dcih_info {
	unsigned char buf_mode;			/* buf mode, 0 for forward, 1 for backward */
	unsigned char is_inited;		/* DCIH driver inited or not */
	unsigned char is_shared;		/* DCIH driver shared with secure driver or not */
	unsigned int dcih_id;			/* DCIH driver id */
	unsigned int buf_size;			/* DCIH driver allocated buf size */
	unsigned long virt_addr;		/* DCIH driver allocated virtual buf addr */
	unsigned long phy_addr;			/* DCIH driver allocated phy buf addr */
	struct completion tee_signal;	/* Wait for the signal from TEE driver (DCIH of TEE->REE) */
	struct completion ree_signal;	/* Notify to REE driver (DCIH of TEE->REE) */
};
#endif

extern int create_nq_buffer(void);
extern unsigned long create_fp_fdrv(int buff_size);
extern unsigned long create_keymaster_fdrv(int buff_size);
extern unsigned long create_gatekeeper_fdrv(int buff_size);
extern unsigned long create_cancel_fdrv(int buff_size);
extern long init_all_service_handlers(void);
extern int register_sched_irq_handler(void);
extern int register_soter_irq_handler(int irq);
extern int register_error_irq_handler(void);
extern int register_fp_ack_handler(void);
extern int register_keymaster_ack_handler(void);
extern int register_bdrv_handler(void);
extern int register_tlog_handler(void);
extern int register_boot_irq_handler(void);
extern int register_switch_irq_handler(void);

extern int register_ut_irq_handler(int irq);

extern struct teei_context *teei_create_context(int dev_count);
extern struct teei_session *teei_create_session(struct teei_context *cont);
extern int teei_client_context_init(void *private_data, void *argp);
extern int teei_client_context_close(void *private_data, void *argp);
extern int teei_client_session_init(void *private_data, void *argp);
extern int teei_client_session_open(void *private_data, void *argp);
extern int teei_client_session_close(void *private_data, void *argp);
extern int teei_client_send_cmd(void *private_data, void *argp);
extern int teei_client_operation_release(void *private_data, void *argp);
extern int teei_client_prepare_encode(void *private_data,
                                      struct teei_client_encode_cmd *enc,
                                      struct teei_encode **penc_context,
                                      struct teei_session **psession);
extern int teei_client_encode_uint32(void *private_data, void *argp);
extern int teei_client_encode_array(void *private_data, void *argp);
extern int teei_client_encode_mem_ref(void *private_data, void *argp);
extern int teei_client_encode_uint32_64bit(void *private_data, void *argp);
extern int teei_client_encode_array_64bit(void *private_data, void *argp);
extern int teei_client_encode_mem_ref_64bit(void *private_data, void *argp);
extern int teei_client_prepare_decode(void *private_data,
                                      struct teei_client_encode_cmd *dec,
                                      struct teei_encode **pdec_context);
extern int teei_client_decode_uint32(void *private_data, void *argp);
extern int teei_client_decode_array_space(void *private_data, void *argp);
extern int teei_client_get_decode_type(void *private_data, void *argp);
extern int teei_client_shared_mem_alloc(void *private_data, void *argp);
extern int teei_client_shared_mem_free(void *private_data, void *argp);
extern int teei_client_close_session_for_service(
        void *private_data,
        struct teei_session *temp_ses);
extern int teei_client_service_exit(void *private_data);
extern void init_tlog_entry(void);
extern int global_fn(void);

extern long create_tlog_thread(unsigned long tlog_virt_addr, unsigned long buff_size);
extern int add_work_entry(int work_type, unsigned long buff);
extern long create_utgate_log_thread(unsigned long tlog_virt_addr, unsigned long buff_size);
extern void init_sched_work_ent(void);
extern void *__teei_client_map_mem(int dev_file_id, unsigned long size, unsigned long user_addr);
extern long __teei_client_open_dev(void);

struct semaphore api_lock;
extern unsigned long fp_buff_addr;
extern unsigned long cancel_message_buff;
extern unsigned long keymaster_buff_addr;
extern unsigned long gatekeeper_buff_addr;

struct work_queue *secure_wq;
struct work_queue *bdrv_wq;

unsigned long fdrv_message_buff;
unsigned long bdrv_message_buff;
unsigned long tlog_message_buff;
unsigned long message_buff;

#endif /* __TEEI_CLIENT_MAIN_H__ */
