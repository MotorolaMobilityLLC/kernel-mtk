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

#include <kpd.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

static u16 kpd_keymap_state[KPD_NUM_MEMS] = {
	0xffff, 0xffff, 0xffff, 0xffff, 0x00ff
};

void kpd_enable(int enable)
{
	if (enable == 1) {
		writew((u16) (enable), KP_EN);
		kpd_info("KEYPAD is enabled\n");
	} else if (enable == 0) {
		writew((u16) (enable), KP_EN);
		kpd_info("KEYPAD is disabled\n");
	}
}

void kpd_get_keymap_state(u16 state[])
{
	state[0] = *(u16 *)KP_MEM1;
	state[1] = *(u16 *)KP_MEM2;
	state[2] = *(u16 *)KP_MEM3;
	state[3] = *(u16 *)KP_MEM4;
	state[4] = *(u16 *)KP_MEM5;
	kpd_info(KPD_SAY "register = %x %x %x %x %x\n", state[0], state[1], state[2], state[3], state[4]);

}


/********************************************************************/
void kpd_init_keymap(u16 keymap[])
{
	int i = 0;

	for (i = 0; i < KPD_NUM_KEYS; i++)
		keymap[i] = kpd_dts_data.kpd_hw_init_map[i];
}

void kpd_init_keymap_state(u16 keymap_state[])
{
	int i = 0;

	for (i = 0; i < KPD_NUM_MEMS; i++)
		keymap_state[i] = kpd_keymap_state[i];
	kpd_info("init_keymap_state done: %x %x %x %x %x!\n", keymap_state[0], keymap_state[1], keymap_state[2],
		 keymap_state[3], keymap_state[4]);
}

/********************************************************************/

void kpd_set_debounce(u16 val)
{
	writew((u16) (val & KPD_DEBOUNCE_MASK), KP_DEBOUNCE);
}

void kpd_double_key_enable(int en)
{
	u16 tmp;

	tmp = *(u16 *)KP_SEL;
	if (en)
		writew((u16) (tmp | KPD_DOUBLE_KEY_MASK), KP_SEL);
	else
		writew((u16) (tmp & ~KPD_DOUBLE_KEY_MASK), KP_SEL);
}

