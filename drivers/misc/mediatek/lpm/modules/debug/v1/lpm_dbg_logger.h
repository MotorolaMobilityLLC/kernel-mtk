/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __LPM_DBG_LOGGER_H__
#define __LPM_DBG_LOGGER_H__

int lpm_logger_init(void);

void lpm_logger_deinit(void);

u32 get_sys_lpm_sleep_time(int index);
u32 get_wakeup_R12_index(void);
char *get_wakeup_R12_source(void);
#endif
