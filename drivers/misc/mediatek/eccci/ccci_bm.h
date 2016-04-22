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

#ifndef __CCCI_BM_H__
#define __CCCI_BM_H__

#include "ccci_core.h"
#include "ccci_config.h"
/*
 * the actually allocated skb's buffer is much bigger than what we request, so when we judge
 * which pool it belongs, the comparision is quite tricky...
 * another trikcy part is, ccci_fsd use CCCI_MTU as its payload size, but it treat both ccci_header
 * and op_id as header, so the total packet size will be CCCI_MTU+sizeof(ccci_header)+sizeof(
 * unsigned int). check port_char's write() and ccci_fsd for detail.
 *
 * beaware, these macros are also used by CLDMA
 */
#define SKB_4K (CCCI_MTU+128)	/* user MTU+CCCI_H+extra(ex. ccci_fsd's OP_ID), for genral packet */
#define SKB_1_5K (CCCI_NET_MTU+16)	/* net MTU+CCCI_H, for network packet */
#define SKB_16 16		/* for struct ccci_header */
#define NET_RX_BUF SKB_4K

#ifdef NET_SKBUFF_DATA_USES_OFFSET
#define skb_size(x) ((x)->end)
#define skb_data_size(x) ((x)->head + (x)->end - (x)->data)
#else
#define skb_size(x) ((x)->end - (x)->head)
#define skb_data_size(x) ((x)->end - (x)->data)
#endif

struct ccci_req_queue {
	unsigned int magic_header;
	struct list_head req_list;
	unsigned int count;
	unsigned int max_len;
	spinlock_t req_lock;
	wait_queue_head_t req_wq;
	unsigned int magic_footer;
};

struct ccci_skb_queue {
	unsigned int magic_header;
	struct sk_buff_head skb_list;
	unsigned int max_len;
	struct work_struct reload_work;
	unsigned char pre_filled;
	unsigned int max_history;
	unsigned int magic_footer;
};

struct sk_buff *ccci_alloc_skb(int size, char from_pool, char blocking);
void ccci_free_skb(struct sk_buff *skb, DATA_POLICY policy);

struct sk_buff *ccci_skb_dequeue(struct ccci_skb_queue *queue);
void ccci_skb_enqueue(struct ccci_skb_queue *queue, struct sk_buff *newsk);
void ccci_skb_queue_init(struct ccci_skb_queue *queue, unsigned int skb_size, unsigned int max_len,
	char fill_now);

struct ccci_request *ccci_alloc_req(DIRECTION dir, int size, char blk1, char blk2);
void ccci_free_req(struct ccci_request *req);
void ccci_dump_req(struct ccci_request *req);

void ccci_mem_dump(int md_id, void *start_addr, int len);
void ccci_cmpt_mem_dump(int md_id, void *start_addr, int len);

#endif				/* __CCCI_BM_H__ */
