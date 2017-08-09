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

#ifndef __SCP_EXCEP_H__
#define __SCP_EXCEP_H__

typedef enum scp_excep_id {
	EXCEP_LOAD_FIRMWARE = 0,
	EXCEP_RESET,
	EXCEP_BOOTUP,
	EXCEP_RUNTIME,
	SCP_NR_EXCEP,
} scp_excep_id;

extern void scp_aed(scp_excep_id type);
extern void scp_aed_reset(scp_excep_id type);
extern void scp_aed_reset_inplace(scp_excep_id type);
extern void scp_get_log(int save);
extern void scp_dump_regs(void);
extern uint32_t scp_dump_pc(void);
extern uint32_t scp_dump_lr(void);
extern void aed_md32_exception_api(const int *log, int log_size, const int *phy,
		int phy_size, const char *detail, const int db_opt);
extern void scp_excep_reset(void);
enum { r0, r1, r2, r3, r12, lr, pc, psr};

typedef struct TaskContextType {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int sp;              /* after pop r0-r3, lr, pc, xpsr                   */
	unsigned int lr;              /* lr before exception                             */
	unsigned int pc;              /* pc before exception                             */
	unsigned int psr;             /* xpsr before exeption                            */
	unsigned int control;         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
	unsigned int exc_return;      /* current lr                                      */
	unsigned int msp;             /* msp                                             */
} TaskContext;

#define CRASH_SUMMARY_LENGTH 12
#define CRASH_MEMORY_LENGTH  (512 * 1024)
#define CRASH_MEMORY_OFFSET  (0x800)

typedef struct MemoryDumpType {
	char crash_summary[CRASH_SUMMARY_LENGTH];
	TaskContext core;
	int flash_length;
	int sram_length;
	char memory[CRASH_MEMORY_LENGTH];
} MemoryDump;

#endif
