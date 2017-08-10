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

#ifndef TEEI_CAPI_H
#define TEEI_CAPI_H

struct teei_contexts_head_t {
	u32 dev_file_cnt;
	struct list_head context_list;
	struct rw_semaphore teei_contexts_sem;
};
extern struct teei_context *teei_create_context(int dev_count);
extern struct teei_session *teei_create_session(struct teei_context *cont);
extern struct teei_contexts_head_t teei_contexts_head;

int teei_client_close_session_for_service(void *private_data, struct teei_session *temp_ses);
int teei_client_service_exit(void *private_data);
int teei_client_context_init(void *private_data, void *argp);
int teei_client_context_close(void *private_data, void *argp);
int teei_client_session_init(void *private_data, void *argp);
int teei_client_session_open(void *private_data, void *argp);
int teei_client_session_close(void *private_data, void *argp);
int teei_client_send_cmd(void *private_data, void *argp);
int teei_client_operation_release(void *private_data, void *argp);
int teei_client_encode_uint32(void *private_data, void *argp);
int teei_client_encode_array(void *private_data, void *argp);
int teei_client_encode_mem_ref(void *private_data, void *argp);
int teei_client_encode_uint32_64bit(void *private_data, void *argp);
int teei_client_encode_array_64bit(void *private_data, void *argp);
int teei_client_encode_mem_ref_64bit(void *private_data, void *argp);
int teei_client_decode_uint32(void *private_data, void *argp);
int teei_client_decode_array_space(void *private_data, void *argp);
int teei_client_get_decode_type(void *private_data, void *argp);
int teei_client_shared_mem_alloc(void *private_data, void *argp);
int teei_client_shared_mem_free(void *private_data, void *argp);

#endif /* end of TEEI_CAPI_H */
