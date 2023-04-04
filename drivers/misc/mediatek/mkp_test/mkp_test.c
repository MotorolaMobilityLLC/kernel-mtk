// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Yee Lee <yee.lee@mediatek.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include <trace/hooks/avc.h>
#include <trace/hooks/creds.h>
#include <trace/hooks/syscall_check.h>
#include <linux/delay.h>
#include "mkp_security.h"
#include "dummy_ksym.h"

#define TEST(unused, func)\
	static int func(struct seq_file *m)

#define DEFINE_TEST(testcase)\
static int testcase##_show(struct seq_file *m, void *v) {return testcase(m);}\
\
static int testcase##_open(struct inode *inode, struct file *file){\
	return single_open(file, testcase##_show, NULL);\
}\
static const struct file_operations testcase##_fops = {\
	.open = testcase##_open,\
	.read = seq_read,\
	.llseek = seq_lseek,\
	.release = single_release,\
};

#define ADD_TEST(dir, testcase)\
debugfs_create_file(#testcase, 0444, dir, NULL, &testcase##_fops);

struct avc_sbuf_content {
	unsigned long avc_node;
	u32 ssid __aligned(8);
	u32 tsid __aligned(8);
	u16 tclass __aligned(8);
	u32 ae_allowed __aligned(8);
} __aligned(8);

struct _cred_sbuf_content {
	kuid_t uid;
	kgid_t gid;
	kuid_t euid;
	kgid_t egid;
	kuid_t fsuid;
	kgid_t fsgid;
	void *security;
};

struct cred_sbuf_content {
	union {
		struct _cred_sbuf_content csc;
		unsigned long args[4];
	};
};

#define mkp_test_msg(fmt, args...)      seq_printf(m, "[mkp_test] "fmt"\n", ##args)

static noinline void __try_to_attack(struct seq_file *m, void *taddr)
{
	*(char *)(taddr) = '0';
	if (*(volatile char *)(taddr) == '0') {
		mkp_test_msg("__try_to_attack succeeded\n");
	} else {
		mkp_test_msg("__try_to_attack failed\n");
	}
}

static void try_to_attack_ko(struct seq_file *m, unsigned long addr)
{
	phys_addr_t phys_addr;
	struct page *tpage;
	void *taddr;

	phys_addr = vmalloc_to_pfn((void *)addr) << PAGE_SHIFT;
	tpage = phys_to_page(phys_addr);
	taddr = vmap(&tpage, 1, VM_MAP, PAGE_KERNEL);
	if (!taddr) {
		seq_puts(m, "abort test\n");
		return;
	}

	__try_to_attack(m, taddr);
}

static void try_to_attack(struct seq_file *m, unsigned long addr)
{
	phys_addr_t phys_addr;
	struct page *tpage;
	void *taddr;

	phys_addr = virt_to_phys((void *)addr);
	tpage = phys_to_page(phys_addr);
	taddr = vmap(&tpage, 1, VM_MAP, PAGE_KERNEL);
	if (!taddr) {
		seq_puts(m, "abort test\n");
		return;
	}

	__try_to_attack(m, taddr);
}

/******************************************************
 * mkp_module_load
 ******************************************************/
TEST(mkp_test, mkp_test_001_ko)
{
	const char *name = "mkp";
	struct module *pMod, *list_mod;
	int ret = -1;

	mkp_test_msg(" [SCN001]: check mkp.ko alive\n");
	pMod = NULL;
	preempt_disable();
	list_for_each_entry(list_mod, THIS_MODULE->list.prev, list){
		if (strcmp(list_mod->name, name) == 0 ){
			pMod = list_mod;
			break;
		}
	}
	preempt_enable();
	mkp_test_msg(" [SCN001]: module address : %lx\n", (unsigned long)pMod);
	if(!pMod)
		return 0;

	ret = pMod->state;
	if(ret==0)
		seq_puts(m, "PASS\n");

	return 0;
}

/******************************************************
 * mkp_self_protection
 ******************************************************/
TEST(mkp_test, mkp_test_002_mkp)
{
	const char *name = "mkp";
	struct module *pMod, *list_mod;
	const struct module_layout* layout;
	unsigned long ro_addr;
	mkp_test_msg(" [SCN002]: protect mkp (no toleration)\n");
	pMod = NULL;
	preempt_disable();
	list_for_each_entry(list_mod, THIS_MODULE->list.prev, list){
		if (strcmp(list_mod->name, name) == 0 ){
			pMod = list_mod;
			break;
		}
	}
	preempt_enable();
	if(!pMod)
		return 0;

	layout = &pMod->core_layout;
	ro_addr = (unsigned long)layout->base+layout->text_size,
	mkp_test_msg(" [SCN002]: rodata address: %lx\n", ro_addr);

	/* Start attack */
	try_to_attack_ko(m, ro_addr);

	/* Dump result */
	seq_puts(m, "KE! should not come here\n");
	return 0;
}

/******************************************************
 * driver_protection
 ******************************************************/
TEST(mkp_test, mkp_test_003_driver)
{
	struct module *pMod = THIS_MODULE;
	const struct module_layout* layout;
	unsigned long ro_addr;
	mkp_test_msg(" [SCN003]: protect driver (no toleration)\n");
	if(!pMod)
		return 0;

	layout = &pMod->core_layout;
	ro_addr = (unsigned long)layout->base+layout->text_size,
	mkp_test_msg(" [SCN003]: driver module rodata address: %lx\n", ro_addr);

	/* Start attack */
	try_to_attack_ko(m, ro_addr);

	/* Dump result */
	seq_puts(m, "KE! should not come here\n");
	return 0;
}
/******************************************************
 * selinux_avc_protection
 ******************************************************/
TEST(mkp_test, mkp_test_004_avc)
{
	int i = 0;
	struct mkp_avc_node node = {
		.ae = {
			.ssid = 0x111,
			.tsid = 0x222,
			.tclass = 0x10,
			.avd = { .allowed = 0x1 },
		 },
	};

	mkp_test_msg("[SCN004]: AVC checks\n");
	//Insert an avc node
	trace_android_vh_selinux_avc_insert((struct avc_node*)&node);
	//modify avc node
	node.ae.tclass=0xAA;

	for(i=0; i<10000; i++) {
		if (i%50==0)
			msleep(10);
		//Lookup the avc node. Trigger error.
		trace_android_vh_selinux_avc_lookup((struct avc_node*)&node, node.ae.ssid, node.ae.tsid, node.ae.tclass);
	}
	seq_puts(m, "Completed\n");
	return 0;
}
/******************************************************
 * task_credential_protection
 ******************************************************/
TEST(mkp_test, mkp_test_005_creds)
{
	struct task_struct *cur = NULL;
	int i;
	struct cred cred = {
		.uid = { .val = 0x1},
		.gid = { .val = 0x1},
		.euid = { .val = 0x1},
		.egid = { .val = 0x1},
		.fsuid = { .val = 0x1},
		.fsgid = { .val = 0x1},
		.security = NULL,
	};

	mkp_test_msg("[SCN005]: Creds checks \n");
	cur = get_current();
	cred.security = cur->cred->security;

	//Commit the fake creds
	trace_android_vh_commit_creds(cur, &cred);

	//loop to call open file
	for(i=0; i<100; i++) {
		if (i%10==0)
			msleep(10);
		//Check creds. Trigger error.
		trace_android_vh_check_file_open(NULL);
	}
	seq_puts(m, "Completed\n");
	return 0;
}
/******************************************************
 * kernel_code_and_rodata_protection
 ******************************************************/

#define TOLERATION_CNT	(3)

static void *p_stext;
static void *p_etext;
static void *p__init_begin;

TEST(mkp_test, mkp_test_006_rodata)
{
	unsigned long addr_start;
	static int cnt = 1;

	mkp_test_msg("[SCN006]: protect kernel RO data (... %d)\n", cnt);
	mkp_get_krn_rodata(&p_etext, &p__init_begin);
	addr_start = (unsigned long)p_etext;

	if(!addr_start)
		return 0;

	mkp_test_msg("[SCN006]: kernel rodata address: %lx\n", addr_start);

	/* Start attack */
	try_to_attack(m, addr_start);

	/* Dump result */
	if (cnt <= TOLERATION_CNT)
		seq_printf(m, "(%d) tolerated\n", cnt++);
	else
		seq_puts(m, "KE! should not come here\n");

	return 0;
}
/******************************************************
 * kernel_pages_protection
 ******************************************************/
TEST(mkp_test, mkp_test_007_kernel)
{
	unsigned long addr_start;
	static int cnt = 1;

	mkp_test_msg("[SCN007]: protect kernel code (... %d)\n", cnt);
	mkp_get_krn_code(&p_stext, &p_etext);
	addr_start = (unsigned long)p_stext;

	if(!addr_start)
		return 0;

	mkp_test_msg("[SCN007]: kernel code address: %lx\n", addr_start);

	/* Start attack */
	try_to_attack(m, addr_start);

	/* Dump result */
	if (cnt <= TOLERATION_CNT)
		seq_printf(m, "(%d) tolerated\n", cnt++);
	else
		seq_puts(m, "KE! should not come here\n");

	return 0;
}

DEFINE_TEST(mkp_test_001_ko)
DEFINE_TEST(mkp_test_002_mkp)
DEFINE_TEST(mkp_test_003_driver)
DEFINE_TEST(mkp_test_004_avc)
DEFINE_TEST(mkp_test_005_creds)
DEFINE_TEST(mkp_test_006_rodata)
DEFINE_TEST(mkp_test_007_kernel)

static struct dentry *dir;
static void add_tests(void)
{
	dir = debugfs_create_dir("mkp_test", NULL);
	if (dir == NULL)
		return;

	ADD_TEST(dir, mkp_test_001_ko);
	ADD_TEST(dir, mkp_test_002_mkp);
	ADD_TEST(dir, mkp_test_003_driver);
	ADD_TEST(dir, mkp_test_004_avc);
	ADD_TEST(dir, mkp_test_005_creds);
	ADD_TEST(dir, mkp_test_006_rodata);
	ADD_TEST(dir, mkp_test_007_kernel);
}

static int __init mkp_test_init(void)
{
	if (mkp_ka_init() != 0) {
		pr_info("%s: failed\n", __func__);
		return 0;
	}

	add_tests();
	pr_info("module loaded\n");
	return 0;
}

static void __exit mkp_test_exit(void)
{
	pr_info("module unloaded\n");
}

module_init(mkp_test_init);
module_exit(mkp_test_exit);
MODULE_LICENSE("GPL");
