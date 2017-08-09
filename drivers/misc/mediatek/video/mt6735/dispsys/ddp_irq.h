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

#ifndef _H_DDP_IRQ_H
#define _H_DDP_IRQ_H

#include "ddp_info.h"
#include "linux/irqreturn.h"

typedef void (*DDP_IRQ_CALLBACK)(DISP_MODULE_ENUM module, unsigned int reg_value);

int disp_register_module_irq_callback(DISP_MODULE_ENUM module, DDP_IRQ_CALLBACK cb);
int disp_unregister_module_irq_callback(DISP_MODULE_ENUM module, DDP_IRQ_CALLBACK cb);

int disp_register_irq_callback(DDP_IRQ_CALLBACK cb);
int disp_unregister_irq_callback(DDP_IRQ_CALLBACK cb);

void disp_register_irq(unsigned int irq_num, char *device_name);
int disp_init_irq(void);

void disp_dump_emi_status(void);

extern unsigned int ovl_complete_irq_cnt[2];
extern unsigned long long rdma_start_time[2];
extern unsigned long long rdma_end_time[2];
extern unsigned int rdma_start_irq_cnt[2];
extern unsigned int rdma_done_irq_cnt[2];
extern unsigned int rdma_underflow_irq_cnt[2];
extern unsigned int rdma_targetline_irq_cnt[2];
extern unsigned int mutex_start_irq_cnt;
extern unsigned int mutex_done_irq_cnt;
extern atomic_t ESDCheck_byCPU;

irqreturn_t disp_irq_handler(int irq, void *dev_id);


#endif
