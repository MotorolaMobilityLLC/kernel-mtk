/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/vmalloc.h>         /* needed by vmalloc */
#include <linux/sysfs.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/workqueue.h>
#include <mt-plat/aee.h>
#include <asm/io.h>
#include <mt-plat/sync_write.h>
#include <linux/mutex.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"


#define AED_DUMP_SIZE	0x4000
#define SCP_LOCK_OFS	0xE0
#define SCP_TCM_LOCK	(1 << 20)

struct scp_aed_cfg {
	int *log;
	int log_size;
	int *phy;
	int phy_size;
	char *detail;
	MemoryDump *pMemoryDump;
	int memory_dump_size;
};

struct scp_status_reg {
	unsigned int pc;
	unsigned int lr;
	unsigned int psp;
	unsigned int sp;
	unsigned int m2h;
	unsigned int h2m;
};

static unsigned char *scp_dump_buffer;
static unsigned int scp_dump_length;
static unsigned int task_context_address;
static struct scp_work_struct scp_aed_work;
static struct scp_status_reg scp_aee_status;
static struct mutex scp_excep_mutex, scp_excep_dump_mutex;

/* An ELF note in memory */
struct memelfnote {
	const char *name;
	int type;
	unsigned int datasz;
	void *data;
};

static int notesize(struct memelfnote *en)
{
	int sz;

	sz = sizeof(struct elf32_note);
	sz += roundup((strlen(en->name) + 1), 4);
	sz += roundup(en->datasz, 4);

	return sz;
}

static uint8_t *storenote(struct memelfnote *men, uint8_t *bufp)
{
	struct elf32_note en;

	en.n_namesz = strlen(men->name) + 1;
	en.n_descsz = men->datasz;
	en.n_type = men->type;

	memcpy(bufp, &en, sizeof(en));
	bufp += sizeof(en);

	memcpy(bufp, men->name, en.n_namesz);
	bufp += en.n_namesz;

	bufp = (uint8_t *) roundup((unsigned long)bufp, 4);
	memcpy(bufp, men->data, men->datasz);
	bufp += men->datasz;

	bufp = (uint8_t *) roundup((unsigned long)bufp, 4);
	return bufp;
}

static uint8_t *core_write_cpu_note(int cpu, struct elf32_phdr *nhdr, uint8_t *bufp)
{
	struct memelfnote notes;
	struct elf32_prstatus prstatus;
	char cpustr[16];

	memset(&prstatus, 0, sizeof(struct elf32_prstatus));
	snprintf(cpustr, sizeof(cpustr), "CPU%d", cpu);
	/* set up the process status */
	notes.name = cpustr;
	notes.type = NT_PRSTATUS;
	notes.datasz = sizeof(struct elf32_prstatus);
	notes.data = &prstatus;

	prstatus.pr_pid = cpu + 1;
	if (task_context_address) {
		memcpy_from_scp((void *)&(prstatus.pr_reg),
				(void *)(SCP_TCM + task_context_address), sizeof(prstatus.pr_reg));
	}
	if (prstatus.pr_reg[15] == 0x0)
		prstatus.pr_reg[15] = SCP_DEBUG_PC_REG;
	if (prstatus.pr_reg[14] == 0x0)
		prstatus.pr_reg[14] = SCP_DEBUG_LR_REG;
	if (prstatus.pr_reg[13] == 0x0)
		prstatus.pr_reg[13] = SCP_DEBUG_PSP_REG;

	nhdr->p_filesz += notesize(&notes);
	return storenote(&notes, bufp);
}

void exception_header_init(void *oldbufp)
{
	struct elf32_phdr *nhdr, *phdr;
	struct elf32_hdr *elf;
	off_t offset = 0;

	uint8_t *bufp = oldbufp;
	uint32_t cpu;

	/* NT_PRPSINFO */
	struct elf32_prpsinfo prpsinfo;
	struct memelfnote notes;

	elf = (struct elf32_hdr *) bufp;
	bufp += sizeof(struct elf32_hdr);
	offset += sizeof(struct elf32_hdr);
	elf_setup_eident(elf->e_ident, ELFCLASS32);
	elf_setup_elfhdr(elf, EM_ARM, struct elf32_hdr, struct elf32_phdr)

	nhdr = (struct elf32_phdr *) bufp;
	bufp += sizeof(struct elf32_phdr);
	offset += sizeof(struct elf32_phdr);
	memset(nhdr, 0, sizeof(struct elf32_phdr));
	nhdr->p_type = PT_NOTE;

	phdr = (struct elf32_phdr *) bufp;
	bufp += sizeof(struct elf32_phdr);
	offset += sizeof(struct elf32_phdr);
	phdr->p_flags = PF_R|PF_W|PF_X;
	phdr->p_offset = CRASH_MEMORY_HEADER_SIZE;
	phdr->p_vaddr = CRASH_MEMORY_OFFSET;
	phdr->p_paddr = CRASH_MEMORY_OFFSET;
	phdr->p_filesz = (SCP_TCM_SIZE - CRASH_MEMORY_OFFSET);
	phdr->p_memsz = (SCP_TCM_SIZE - CRASH_MEMORY_OFFSET);
	phdr->p_align = 0;
	phdr->p_type = PT_LOAD;

	nhdr->p_offset = offset;

	/* set up the process info */
	notes.name = CORE_STR;
	notes.type = NT_PRPSINFO;
	notes.datasz = sizeof(struct elf32_prpsinfo);
	notes.data = &prpsinfo;

	memset(&prpsinfo, 0, sizeof(struct elf32_prpsinfo));
	prpsinfo.pr_state = 0;
	prpsinfo.pr_sname = 'R';
	prpsinfo.pr_zomb = 0;
	prpsinfo.pr_gid = prpsinfo.pr_uid = 0x0;
	strlcpy(prpsinfo.pr_fname, "freertos8", sizeof(prpsinfo.pr_fname));
	strlcpy(prpsinfo.pr_psargs, "freertos8", ELF_PRARGSZ);

	nhdr->p_filesz += notesize(&notes);
	bufp = storenote(&notes, bufp);

	/* Store pre-cpu backtrace */
	for (cpu = 0; cpu < 1; cpu++)
		bufp = core_write_cpu_note(cpu, nhdr, bufp);
}

static void scp_dump_buffer_set(unsigned char *buf, unsigned int length)
{
	if (length == 0) {
		vfree(buf);
		return;
	}

	mutex_lock(&scp_excep_dump_mutex);
	vfree(scp_dump_buffer);

	scp_dump_buffer = buf;
	scp_dump_length = length;
	mutex_unlock(&scp_excep_dump_mutex);
}
/*
 * return last lr for debugging
 */
uint32_t scp_dump_lr(void)
{
	if (is_scp_ready())
		return SCP_DEBUG_LR_REG;
	else
		return 0xFFFFFFFF;
}
/*
 * return last pc for debugging
 */
uint32_t scp_dump_pc(void)
{
	if (is_scp_ready())
		return SCP_DEBUG_PC_REG;
	else
		return 0xFFFFFFFF;
}
/*
 * dump scp register for debugging
 */
void scp_dump_regs(void)
{
	if (is_scp_ready()) {
		pr_err("[SCP} SCP is alive\n");
		pr_err("[SCP] SCP_DEBUG_PC_REG:0x%x\n", SCP_DEBUG_PC_REG);
		pr_err("[SCP] SCP_DEBUG_LR_REG:0x%x\n", SCP_DEBUG_LR_REG);
		pr_err("[SCP] SCP_DEBUG_PSP_REG:0x%x\n", SCP_DEBUG_PSP_REG);
		pr_err("[SCP] SCP_DEBUG_SP_REG:0x%x\n", SCP_DEBUG_SP_REG);
	} else {
		pr_err("[SCP} SCP is dead\n");
	}
}

/*
 * save scp register when scp crash
 * these data will be used to generate EE
 */
void scp_aee_last_reg(void)
{
	pr_debug("scp_aee_last_reg\n");

	scp_aee_status.pc = SCP_DEBUG_PC_REG;
	scp_aee_status.lr = SCP_DEBUG_LR_REG;
	scp_aee_status.psp = SCP_DEBUG_PSP_REG;
	scp_aee_status.sp = SCP_DEBUG_SP_REG;
	scp_aee_status.m2h = SCP_TO_HOST_REG;
	scp_aee_status.h2m = HOST_TO_SCP_REG;

	pr_debug("scp_aee_last_reg end\n");
}

/*
 * generate exception log for aee
 * @param buf:      log buffer
 * @param buf_len:  size for log buffer
 * @return:         length of log
 */
static int scp_aee_dump(char *buf, ssize_t buf_len)
{
	/* TODO: FIXME */
	ssize_t len;

	if (!buf)
		return 0;

#define neg_force_zero(x)	((((ssize_t)(x)) > 0) ? (x) : 0)
	len = 0;

	pr_debug("scp_aee_dump\n");
	len += snprintf(buf + len, neg_force_zero(buf_len - len), "scp pc=0x%08x, lr=0x%08x, psp=0x%08x, sp=0x%08x\n",
	scp_aee_status.pc, scp_aee_status.lr, scp_aee_status.psp, scp_aee_status.sp);
	len += snprintf(buf + len, neg_force_zero(buf_len - len), "scp to host irq = 0x%08x\n", scp_aee_status.m2h);
	len += snprintf(buf + len, neg_force_zero(buf_len - len), "host to scp irq = 0x%08x\n", scp_aee_status.h2m);

	len += snprintf(buf + len, neg_force_zero(buf_len - len), "wdt en=%d, count=0x%08x\n",
			(SCP_WDT_REG & 0x10000000) ? 1 : 0,
			(SCP_WDT_REG & 0xFFFFF));
	len += snprintf(buf + len, neg_force_zero(buf_len - len), "\n");

#undef neg_force_zero

	pr_debug("scp_aee_dump end\n");

	return len;
}

void scp_excep_reset(void)
{
	task_context_address = 0;
}
static unsigned int scp_crash_dump(MemoryDump *pMemoryDump)
{
	unsigned int lock;
	unsigned int *reg;

	/* check SRAM lock */
	reg = (unsigned int *) (scpreg.cfg + SCP_LOCK_OFS);
	lock = *reg;
	*reg &= ~SCP_TCM_LOCK;
	dsb(SY);
	if ((*reg & SCP_TCM_LOCK)) {
		pr_debug("unlock failed, skip dump\n");
		return 0;
	}

	exception_header_init(pMemoryDump);
	memcpy_from_scp((void *)&(pMemoryDump->memory),
			(void *)(SCP_TCM + CRASH_MEMORY_OFFSET), (SCP_TCM_SIZE - CRASH_MEMORY_OFFSET));
	*reg = lock;
	dsb(SY);

	return sizeof(*pMemoryDump);
}
/*
 * generate aee argument without dump scp register
 * @param aed_str:  exception description
 * @param aed:      struct to store argument for aee api
 */
static void scp_prepare_aed(char *aed_str, struct scp_aed_cfg *aed)
{
	char *detail, *log;
	u8 *phy, *ptr;
	u32 log_size, phy_size;

	pr_debug("scp_prepare_aed\n");

	detail = vmalloc(SCP_AED_STR_LEN);
	ptr = detail;
	memset(detail, 0, SCP_AED_STR_LEN);
	snprintf(detail, SCP_AED_STR_LEN, "%s\n", aed_str);
	detail[SCP_AED_STR_LEN - 1] = '\0';

	log_size = 0;
	log = NULL;

	phy_size = 0;
	phy = NULL;

	aed->log = (int *)log;
	aed->log_size = log_size;
	aed->phy = (int *)phy;
	aed->phy_size = phy_size;
	aed->detail = detail;

	pr_debug("scp_prepare_aed end\n");
}

/*
 * generate aee argument with scp register dump
 * @param aed_str:  exception description
 * @param aed:      struct to store argument for aee api
 */
static void scp_prepare_aed_dump(char *aed_str, struct scp_aed_cfg *aed)
{
	u8 *detail, *log;
	u8 *phy, *ptr;
	u32 log_size, phy_size;
	u32 memory_dump_size;
	MemoryDump *pMemoryDump = NULL;


	pr_debug("scp_prepare_aed_dump: %s\n", aed_str);

	scp_aee_last_reg();

	detail = vmalloc(SCP_AED_STR_LEN);
	ptr = detail;
	memset(detail, 0, SCP_AED_STR_LEN);
	snprintf(detail, SCP_AED_STR_LEN, "%s scp pc=0x%08x, lr=0x%08x, psp=0x%08x, sp=0x%08x\n",
	aed_str, scp_aee_status.pc, scp_aee_status.lr, scp_aee_status.psp, scp_aee_status.sp);
	detail[SCP_AED_STR_LEN - 1] = '\0';


	log_size = AED_DUMP_SIZE; /* 16KB */
	log = vmalloc(log_size);
	if (!log) {
		pr_debug("ap allocate log buffer fail, size=0x%x\n", log_size);
		log_size = 0;
	} else {
		memset(log, 0, log_size);

		log_size = scp_aee_dump(log, log_size);

		/* print log in kernel */
		pr_debug("%s", log);
	}

	phy_size = SCP_AED_PHY_SIZE; /* SCP_CFGREG_SIZE+SCP_TCM_SIZE */
	phy = vmalloc(phy_size);
	if (!phy) {
		pr_debug("ap allocate phy buffer fail, size=0x%x\n", phy_size);
		phy_size = 0;
	} else {
		ptr = phy;

		memcpy_from_scp((void *)ptr, (void *)SCP_BASE, SCP_CFGREG_SIZE);
		ptr += SCP_CFGREG_SIZE;

		memcpy_from_scp((void *)ptr, (void *)SCP_TCM, SCP_TCM_SIZE);
		ptr += SCP_TCM_SIZE;
	}
	memory_dump_size = sizeof(*pMemoryDump);
	ptr = vmalloc(memory_dump_size);
	if (!ptr) {
		pr_debug("ap allocate pMemoryDump buffer fail, size=0x%x\n", memory_dump_size);
		memory_dump_size = 0;
	} else {
		pr_debug("[SCP] memory dump:0x%llx\n", (unsigned long long)ptr);
		pMemoryDump = (MemoryDump *) ptr;

		memset(pMemoryDump, 0x0, sizeof(*pMemoryDump));
		memory_dump_size = scp_crash_dump(pMemoryDump);
	}
	scp_dump_buffer_set((unsigned char *) pMemoryDump, memory_dump_size);

	aed->log = (int *)log;
	aed->log_size = log_size;
	aed->phy = (int *)phy;
	aed->phy_size = phy_size;
	aed->detail = detail;
	aed->pMemoryDump = (MemoryDump *) pMemoryDump;
	aed->memory_dump_size = memory_dump_size;

	pr_debug("scp_prepare_aed_dump end\n");
}

/*
 * generate an exception according to exception type
 * @param type: exception type
 */
void scp_aed(scp_excep_id type)
{
	struct scp_aed_cfg aed;

	mutex_lock(&scp_excep_mutex);

	if (type == EXCEP_RUNTIME)
		type = (is_scp_ready()) ? EXCEP_RUNTIME : EXCEP_BOOTUP;

		switch (type) {
		case EXCEP_LOAD_FIRMWARE:
			scp_prepare_aed("scp firmware load exception", &aed);
			break;
		case EXCEP_RESET:
			scp_prepare_aed_dump("scp reset exception", &aed);
			break;
		case EXCEP_BOOTUP:
			scp_prepare_aed_dump("scp boot exception", &aed);
			scp_get_log(1);
			break;
		case EXCEP_RUNTIME:
			scp_prepare_aed_dump("scp runtime exception", &aed);
			scp_get_log(1);
			break;
		default:
			scp_prepare_aed_dump("scp unknown exception", &aed);
			scp_get_log(1);
			break;
	}

	pr_debug("%s", aed.detail);

	/* TODO: apply new scp aed api here */
	aed_scp_exception_api(aed.log, aed.log_size, aed.phy, aed.phy_size, aed.detail, DB_OPT_DEFAULT);
	vfree(aed.detail);
	vfree(aed.log);
	vfree(aed.phy);
	pr_debug("[SCP] scp exception dump is done\n");

	mutex_unlock(&scp_excep_mutex);
}

/*
 * generate an exception and reset scp right now
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param type: exception type
 */
void scp_aed_reset_inplace(scp_excep_id type)
{
	scp_aed(type);

	/* workaround for QA, not reset SCP in WDT */
	if (type == EXCEP_RUNTIME) {
		if (is_scp_ready())
			return;
	}

	reset_scp(1);
}

/*
 * callback function for work struct
 * generate an exception and reset scp
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_aed_reset_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	scp_excep_id type = (scp_excep_id) sws->flags;

	scp_aed_reset_inplace(type);
}

/* IPI for ramdump config
 * @param id:   IPI id
 * @param data: IPI data
 * @param len:  IPI data length
 */
static void scp_ram_dump_ipi_handler(int id, void *data, unsigned int len)
{
	task_context_address = *(unsigned int *)data;
	pr_debug("[SCP] get task_context_address: 0x%x\n", task_context_address);
}
/*
 * schedule a work to generate an exception and reset scp
 * @param type: exception type
 */
void scp_aed_reset(scp_excep_id type)
{
	scp_aed_work.flags = (unsigned int) type;
	scp_schedule_work(&scp_aed_work);
}

static ssize_t scp_dump_show(struct file *filep, struct kobject *kobj, struct bin_attribute *attr,
		char *buf, loff_t offset, size_t size)
{
	unsigned int length = 0;

	mutex_lock(&scp_excep_dump_mutex);

	if (offset >= 0 && offset < scp_dump_length) {
		if ((offset + size) > scp_dump_length)
			size = scp_dump_length - offset;

		memcpy(buf, scp_dump_buffer + offset, size);
		length = size;
	}

	mutex_unlock(&scp_excep_dump_mutex);

	return length;
}

struct bin_attribute bin_attr_scp_dump = {
	.attr = {
		.name = "scp_dump",
		.mode = 0444,
	},
	.size = 0,
	.read = scp_dump_show,
};

/*
 * init a work struct
 */
void scp_excep_init(void)
{
	mutex_init(&scp_excep_mutex);
	mutex_init(&scp_excep_dump_mutex);
	INIT_WORK(&scp_aed_work.work, scp_aed_reset_ws);

	scp_excep_reset();
	scp_dump_buffer = NULL;
	scp_dump_length = 0;
}
/*
 * ram dump init
 */
void scp_ram_dump_init(void)
{
	scp_ipi_registration(IPI_RAM_DUMP, scp_ram_dump_ipi_handler, "ramdump");
	pr_debug("[SCP] ram_dump_init() done\n");
}
