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
extern char *scp_get_last_log(void);
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
#define CRASH_MEMORY_HEADER_SIZE  (8 * 1024)
#define CRASH_MEMORY_LENGTH  (512 * 1024)
#define CRASH_MEMORY_OFFSET  (0x800)

#include <linux/elf.h>
#define ELF_NGREGS 18
#define CORE_STR "CORE"
#define ELF_PRARGSZ 80
#define ELF_CORE_EFLAGS	0
#define EM_ARM 40
static inline void elf_setup_eident(unsigned char e_ident[EI_NIDENT], unsigned char elfclasz)
{
	memcpy(e_ident, ELFMAG, SELFMAG);
	e_ident[EI_CLASS] = elfclasz;
	e_ident[EI_DATA] = ELFDATA2LSB;
	e_ident[EI_VERSION] = EV_CURRENT;
	e_ident[EI_OSABI] = ELFOSABI_NONE;
	memset(e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
}

#define elf_setup_elfhdr(elfp, machid, elfhdr_t, elfphdr_t) \
	do { \
		elfp->e_type = ET_CORE;				 \
		elfp->e_machine = machid;                            \
		elfp->e_version = EV_CURRENT;			 \
		elfp->e_entry = 0;                                   \
		elfp->e_phoff = sizeof(elfhdr_t);                    \
		elfp->e_shoff = 0;                                   \
		elfp->e_flags = ELF_CORE_EFLAGS;                     \
		elfp->e_ehsize = sizeof(elfhdr_t);                   \
		elfp->e_phentsize = sizeof(elfphdr_t);		 \
		elfp->e_phnum = 2;                                   \
		elfp->e_shentsize = 0;				 \
		elfp->e_shnum = 0;                                   \
		elfp->e_shstrndx = 0;				 \
	} while (0)

struct elf_siginfo {
	int	si_signo;
	int	si_code;
	int	si_errno;
};

typedef struct elf32_note_t {
	Elf32_Word   n_namesz;       /* Name size */
	Elf32_Word   n_descsz;       /* Content size */
	Elf32_Word   n_type;         /* Content type */
} Elf_Nhdr;

struct elf32_timeval {
	int32_t tv_sec;
	int32_t tv_usec;
};

struct elf32_prstatus {
	struct elf_siginfo pr_info;
	short pr_cursig;
	uint32_t pr_sigpend;
	uint32_t pr_sighold;

	int32_t pr_pid;
	int32_t pr_ppid;
	int32_t pr_pgrp;

	int32_t pr_sid;
	struct elf32_timeval pr_utime;
	struct elf32_timeval pr_stime;
	struct elf32_timeval pr_cutime;
	struct elf32_timeval pr_cstime;

	uint32_t pr_reg[ELF_NGREGS];

	int32_t pr_fpvalid;
};

struct elf32_prpsinfo {
	char pr_state;
	char pr_sname;
	char pr_zomb;
	char pr_nice;
	uint32_t pr_flag;

	uint16_t pr_uid;
	uint16_t pr_gid;

	int32_t pr_pid;
	int32_t pr_ppid;
	int32_t pr_pgrp;
	int32_t pr_sid;

	char pr_fname[16];
	char pr_psargs[ELF_PRARGSZ];
};

typedef struct MemoryDumpType {
	struct elf32_hdr elf;
	struct elf32_phdr nhdr;
	struct elf32_phdr phdr;
	char notes[CRASH_MEMORY_HEADER_SIZE
		-sizeof(struct elf32_hdr)
		-sizeof(struct elf32_phdr)
		-sizeof(struct elf32_phdr)];
	/*ram dump total header size (elf+nhdr+phdr) must be fixed at CRASH_MEMORY_HEADER_SIZE */
	char memory[CRASH_MEMORY_LENGTH];
} MemoryDump;
#endif
