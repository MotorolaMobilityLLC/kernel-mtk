/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <mtk_spm_sodi_cmdq.h>

/*
 * GCE write spm SW2SPM_MAILBOX3 (0x10B20E2C)
 *     [15:4] : Key, must write 9CE
 *     [1]: CG mode
 *     [0]: Need Bus clock
 * Then polling SPM2SW_MAILBOX_3 (0x10B20E3C) = 0
 */

void exit_pd_by_cmdq(struct cmdqRecStruct *handler)
{
	/* FIXME: */
#if 0
	/* Switch to CG mode */
	cmdqRecWrite(handler, 0x10B20E2C, 0x9ce2, 0xffff);
	/* Polling ack */
	cmdqRecPoll(handler, 0x10B20E3C, 0x0, ~0);
#endif
}

void enter_pd_by_cmdq(struct cmdqRecStruct *handler)
{
	/* FIXME: */
#if 0
	/* Switch to PD mode */
	cmdqRecWrite(handler, 0x10B20E2C, 0x9ce0, 0xffff);
#endif
}
