/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (c) 2019 MediaTek Inc.
*/

#ifndef _LCD_BIAS_H_
#define _LCD_BIAS_H_

void tp_reset_en(unsigned int en);
void lcd_bias_set_vspn(unsigned int en, unsigned int seq, unsigned int value);
void lcd_bias_set_vsp(unsigned int en);
void lcd_bias_set_vsn(unsigned int en);

#endif

