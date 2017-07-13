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

#ifndef KPD_HAL_H
#define KPD_HAL_H

/* Keypad registers */
extern void __iomem *kp_base;

#define KP_STA			(kp_base + 0x0000)
#define KP_MEM1			(kp_base + 0x0004)
#define KP_MEM2			(kp_base + 0x0008)
#define KP_MEM3			(kp_base + 0x000c)
#define KP_MEM4			(kp_base + 0x0010)
#define KP_MEM5			(kp_base + 0x0014)
#define KP_DEBOUNCE		(kp_base + 0x0018)
#define KP_SCAN_TIMING		(kp_base + 0x001C)
#define KP_SEL			(kp_base + 0x0020)
#define KP_EN			(kp_base + 0x0024)

#define KP_COL0_SEL             (1 << 10)
#define KP_COL1_SEL             (1 << 11)
#define KP_COL2_SEL             (1 << 12)

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)
#define KPD_DOUBLE_KEY_MASK	(1U << 0)

/*function define*/
void kpd_enable(int enable);
void kpd_get_keymap_state(u16 state[]);
void kpd_set_debounce(u16 val);
void kpd_init_keymap(u16 keymap[]);
void kpd_init_keymap_state(u16 keymap_state[]);
void kpd_double_key_enable(int en);


extern struct input_dev *kpd_input_dev;

#define KPD_NUM_MEMS	5U
#define KPD_MEM5_BITS	8U

#define KPD_NUM_KEYS	72U	/* 4 * 16 + KPD_MEM5_BITS */
#endif
