#include "mt_spm_sodi_cmdq.h"

void exit_pd_by_cmdq(cmdqRecHandle handler)
{
	/* Switch to CG mode */
	cmdqRecWrite(handler, 0x100062B0, 0x2, ~0);
	/* Polling EMI ACK */
	cmdqRecPoll(handler, 0x1000611C, 0x10000, 0x10000);
}

void enter_pd_by_cmdq(cmdqRecHandle handler)
{
	/* Switch to PD mode */
	cmdqRecWrite(handler, 0x100062B0, 0x0, 0x2);
}
