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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>

#include "mtk_power_gs.h"

#define HEX_FMT "0x%08x"
#define SIZEOF_SNAPSHOT(g) (sizeof(struct snapshot) + sizeof(unsigned int) * (g->nr_golden_setting - 1))
#define DEBUG_BUF_SIZE 2000
static char buf[DEBUG_BUF_SIZE] = { 0 };
static struct base_remap _base_remap;
static struct pmic_manual_dump _pmic_manual_dump;

static bool _is_pmic_addr(unsigned int addr)
{
	return (addr >> 16) ? 0 : 1;
}

static u16 gs_pmic_read(u16 reg)
{
	u32 ret = 0;
	u32 reg_val = 0;

	ret = pmic_read_interface_nolock(reg, &reg_val, 0xFFFF, 0x0);

	return (u16)reg_val;
}

static void _golden_setting_enable(struct golden *g)
{
	if (g) {
		g->buf_snapshot = (struct snapshot *) &(g->buf_golden_setting[g->nr_golden_setting]);
		g->max_nr_snapshot = (g->buf_size - sizeof(struct golden_setting) * g->nr_golden_setting) /
			SIZEOF_SNAPSHOT(g);
		g->snapshot_head = 0;
		g->snapshot_tail = 0;

		g->is_golden_log = 1;
	}
}

static void _golden_setting_disable(struct golden *g)
{
	if (g) {
		g->is_golden_log = 0;

		g->func[0] = '\0';

		g->buf_golden_setting = (struct golden_setting *)g->buf;
		g->nr_golden_setting = 0;
		g->max_nr_golden_setting = g->buf_size / 3 / sizeof(struct golden_setting);
	}
}

static void _golden_setting_set_mode(struct golden *g, print_mode mode)
{
	g->mode = mode;
}

static void _golden_setting_init(struct golden *g, unsigned int *buf, unsigned int buf_size)
{
	if (g && buf) {
		g->mode = MODE_NORMAL;

		g->buf = buf;
		g->buf_size = buf_size;

		_golden_setting_disable(g);
	}
}

static void _golden_setting_add(struct golden *g, unsigned int addr, unsigned int mask, unsigned golden_val)
{
	if (g && 0 == g->is_golden_log && g->nr_golden_setting < g->max_nr_golden_setting && mask != 0) {
		g->buf_golden_setting[g->nr_golden_setting].addr = addr;
		g->buf_golden_setting[g->nr_golden_setting].mask = mask;
		g->buf_golden_setting[g->nr_golden_setting].golden_val = golden_val;

		g->nr_golden_setting++;
	}
}

static void __iomem *_golden_io_phys_to_virt(unsigned int addr)
{
	unsigned int base = addr & (~(unsigned long)REMAP_SIZE_MASK);
	unsigned int offset = addr & (unsigned long)REMAP_SIZE_MASK;

	if (!_golden.phy_base || _golden.phy_base != base) {
		if (_golden.io_base)
			iounmap(_golden.io_base);

		_golden.phy_base = base;
		_golden.io_base = ioremap_nocache(_golden.phy_base, REMAP_SIZE_MASK+1);

		if (!_golden.io_base)
			pr_warn("warning: ioremap_nocache(0x%x, 0x%x) return NULL\n", base, REMAP_SIZE_MASK+1);
	}

	return (_golden.io_base + offset);
}

static int _is_snapshot_full(struct golden *g)
{
	if (g->snapshot_head + 1 == g->snapshot_tail || g->snapshot_head + 1 == g->snapshot_tail + g->max_nr_snapshot)
		return 1;
	else
		return 0;
}

static int _is_snapshot_empty(struct golden *g)
{
	if (g->snapshot_head == g->snapshot_tail)
		return 1;
	else
		return 0;
}

static struct snapshot *_snapshot_produce(struct golden *g)
{
	if (g && !_is_snapshot_full(g)) {
		int idx = g->snapshot_head++;

		if (g->snapshot_head == g->max_nr_snapshot)
			g->snapshot_head = 0;

		return (struct snapshot *)((size_t)(g->buf_snapshot) + SIZEOF_SNAPSHOT(g) * idx);
	} else
		return NULL;
}

static struct snapshot *_snapshot_consume(struct golden *g)
{
	if (g && !_is_snapshot_empty(g)) {
		int idx = g->snapshot_tail++;

		if (g->snapshot_tail == g->max_nr_snapshot)
			g->snapshot_tail = 0;

		is_already_snap_shot = false;

		return (struct snapshot *)((size_t)(g->buf_snapshot) + SIZEOF_SNAPSHOT(g) * idx);
	} else
		return NULL;
}

int _snapshot_golden_setting(struct golden *g, const char *func, const unsigned int line)
{
	struct snapshot *snapshot;
	int i;

	snapshot = _snapshot_produce(g);

	if (g && 1 == g->is_golden_log && snapshot &&
	    (g->func[0] == '\0' || (!strcmp(g->func, func) && ((g->line == line) || (g->line == 0))))) {
		snapshot->func = func;
		snapshot->line = line;

		for (i = 0; i < g->nr_golden_setting; i++) {
			if (_golden.mode == MODE_APPLY) {
				_golden_write_reg(g->buf_golden_setting[i].addr,
						  g->buf_golden_setting[i].mask,
						  g->buf_golden_setting[i].golden_val);
			}

			snapshot->reg_val[i] = _golden_read_reg(g->buf_golden_setting[i].addr);
		}

		is_already_snap_shot = true;

		return 0;
	} else
		return -1;
}

static int _parse_mask_val(char *buf, unsigned int *mask, unsigned int *golden_val)
{
	unsigned int i, bit_shift;
	unsigned int mask_result;
	unsigned int golden_val_result;

	for (i = 0, bit_shift = 1 << 31, mask_result = 0, golden_val_result = 0; bit_shift > 0;) {
		switch (buf[i]) {
		case '1':
			golden_val_result += bit_shift;

		case '0':
			mask_result += bit_shift;

		case 'x':
		case 'X':
			bit_shift >>= 1;

		case '_':
			break;

		default:
			return -1;
		}

		i++;
	}

	*mask = mask_result;
	*golden_val = golden_val_result;

	return 0;
}

static char *_gen_mask_str(const unsigned int mask, const unsigned int reg_val)
{
	static char _mask_str[42];
	unsigned int i, bit_shift;

	strncpy(_mask_str, "0bxxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx", 42);
	for (i = 2, bit_shift = 1 << 31; bit_shift > 0;) {
		switch (_mask_str[i]) {
		case '_':
			break;

		default:
			if (0 == (mask & bit_shift))
				_mask_str[i] = 'x';
			else if (0 == (reg_val & bit_shift))
				_mask_str[i] = '0';
			else
				_mask_str[i] = '1';

		case '\0':
			bit_shift >>= 1;
			break;
		}

		i++;
	}

	return _mask_str;
}

static char *_gen_diff_str(const unsigned int mask, const unsigned int golden_val, const unsigned int reg_val)
{
	static char _diff_str[42];
	unsigned int i, bit_shift;

	strncpy(_diff_str, "0b    _    _    _    _    _    _    _    ", 42);
	for (i = 2, bit_shift = 1 << 31; bit_shift > 0;) {
		switch (_diff_str[i]) {
		case '_':
			break;

		default:
			if (0 != ((golden_val ^ reg_val) & mask & bit_shift))
				_diff_str[i] = '^';
			else
				_diff_str[i] = ' ';

		case '\0':
			bit_shift >>= 1;
			break;
		}

		i++;
	}

	return _diff_str;
}

static char *_gen_color_str(const unsigned int mask, const unsigned int golden_val, const unsigned int reg_val)
{
#define FC "\e[41m"
#define EC "\e[m"
#define XXXX FC "x" EC FC "x" EC FC "x" EC FC "x" EC
	static char _clr_str[300];
	unsigned int i, bit_shift;

	strncpy(_clr_str, "0b"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX, 300);
	for (i = 2, bit_shift = 1 << 31; bit_shift > 0;) {
		switch (_clr_str[i]) {
		case '_':
			break;

		default:
			if (0 != ((golden_val ^ reg_val) & mask & bit_shift))
				_clr_str[i + 3] = '1';
			else
				_clr_str[i + 3] = '0';

			if (0 == (mask & bit_shift))
				_clr_str[i + 5] = 'x';
			else if (0 == (reg_val & bit_shift))
				_clr_str[i + 5] = '0';
			else
				_clr_str[i + 5] = '1';

			i += strlen(EC) + strlen(FC);

		case '\0':
			bit_shift >>= 1;
			break;
		}

		i++;
	}

	return _clr_str;

#undef FC
#undef EC
#undef XXXX
}

static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

 out:
	free_page((unsigned long)buf);

	return NULL;
}

static int golden_test_proc_show(struct seq_file *m, void *v)
{
	static int idx;
	int i = 0;

	idx = 0;

	if (_golden.is_golden_log == 0) {
		for (i = 0; i < _golden.nr_golden_setting; i++) {
			seq_printf(m, ""HEX_FMT" "HEX_FMT" "HEX_FMT"\n",
				   _golden.buf_golden_setting[i].addr,
				   _golden.buf_golden_setting[i].mask,
				   _golden.buf_golden_setting[i].golden_val
				   );
		}
	}

	if (_golden.nr_golden_setting == 0) {
		seq_puts(m, "\n********** golden_test help *********\n");
		seq_puts(m, "1.   disable snapshot:                  echo disable > /proc/clkmgr/golden_test\n");
		seq_puts(m, "2.   insert golden setting (tool mode): echo 0x10000000 (addr) 0bxxxx_xxxx_xxxx_xxxx_0001_0100_1001_0100 (mask & golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(2.) insert golden setting (hex mode):  echo 0x10000000 (addr) 0xFFFF (mask) 0x1494 (golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(2.) insert golden setting (dec mode):  echo 268435456 (addr) 65535 (mask) 5268 (golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "3.   set filter:                        echo filter func_name [line_num] > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(3.) disable filter:                    echo filter > /proc/clkmgr/golden_test\n");
		seq_puts(m, "4.   enable snapshot:                   echo enable > /proc/clkmgr/golden_test\n");
		seq_puts(m, "5.   set compare mode:                  echo compare > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set apply mode:                    echo apply > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set color mode:                    echo color > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set diff mode:                     echo color > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) disable compare/apply/color mode:  echo normal > /proc/clkmgr/golden_test\n");
		seq_puts(m, "6.   set register value (normal mode):  echo set 0x10000000 (addr) 0x13201494 (reg val) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(6.) set register value (mask mode):    echo set 0x10000000 (addr) 0xffff (mask) 0x13201494 (reg val) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(6.) set register value (bit mode):     echo set 0x10000000 (addr) 0 (bit num) 1 (reg val) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "7.   dump suspend comapare:             echo dump_suspend > /proc/clkmgr/golden_test\n");
		seq_puts(m, "8.   dump dpidle comapare:              echo dump_dpidle > /proc/clkmgr/golden_test\n");
		seq_puts(m, "9.   dump sodi comapare:                echo dump_sodi > /proc/clkmgr/golden_test\n");
	} else {
		static struct snapshot *snapshot;

		if (!strcmp(_golden.func, __func__) && (_golden.line == 0))
			snapshot_golden_setting(__func__, 0);

		while ((idx != 0) || ((snapshot = _snapshot_consume(&_golden)) != NULL)) {
			if (idx == 0)
				seq_printf(m, "// @ %s():%d\n", snapshot->func, snapshot->line);

			for (i = idx, idx = 0; i < _golden.nr_golden_setting; i++) {

				if (_golden.mode == MODE_NORMAL
				    || ((_golden.buf_golden_setting[i].mask & _golden.buf_golden_setting[i].golden_val)
					!= (_golden.buf_golden_setting[i].mask & snapshot->reg_val[i])
					)
				    ) {
					if (_golden.mode == MODE_COLOR) {
						seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
							   _golden.buf_golden_setting[i].addr,
							   _golden.buf_golden_setting[i].mask,
							   snapshot->reg_val[i],
							   _gen_color_str(_golden.buf_golden_setting[i].mask,
									  _golden.buf_golden_setting[i].golden_val,
									  snapshot->reg_val[i]));
					} else if (_golden.mode == MODE_DIFF) {
						seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
							   _golden.buf_golden_setting[i].addr,
							   _golden.buf_golden_setting[i].mask,
							   snapshot->reg_val[i],
							   _gen_mask_str(_golden.buf_golden_setting[i].mask,
									 snapshot->reg_val[i]));

						seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
							   _golden.buf_golden_setting[i].addr,
							   _golden.buf_golden_setting[i].mask,
							   _golden.buf_golden_setting[i].golden_val,
							   _gen_diff_str(_golden.buf_golden_setting[i].mask,
									 _golden.buf_golden_setting[i].golden_val,
									 snapshot->reg_val[i]));
					} else
						seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\n",
							   _golden.buf_golden_setting[i].addr,
							   _golden.buf_golden_setting[i].mask,
							   snapshot->reg_val[i]);
				}
			}
		}
	}

	mt_power_gs_sp_dump();

	return 0;
}

static ssize_t golden_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	char cmd[64];
	struct phys_to_virt_table *table;
	unsigned int base;
	unsigned int addr;
	unsigned int mask;
	unsigned int golden_val;

	/* set golden setting (hex mode) */
	if (sscanf(buf, "0x%x 0x%x 0x%x", &addr, &mask, &golden_val) == 3)
		_golden_setting_add(&_golden, addr, mask, golden_val);
	/* set golden setting (dec mode) */
	else if (sscanf(buf, "%d %d %d", &addr, &mask, &golden_val) == 3)
		_golden_setting_add(&_golden, addr, mask, golden_val);
	/* set filter (func + line) */
	/* XXX: 63 = sizeof(_golden.func) - 1 */
	else if (sscanf(buf, "filter %63s %d", _golden.func, &_golden.line) == 2)
		;
	/* set filter (func) */
	/* XXX: 63 = sizeof(_golden.func) - 1 */
	else if (sscanf(buf, "filter %63s", _golden.func) == 1)
		_golden.line = 0;
	/* set golden setting (mixed mode) */
	/* XXX: 63 = sizeof(cmd) - 1 */
	else if (sscanf(buf, "0x%x 0b%63s", &addr, cmd) == 2) {
		if (!_parse_mask_val(cmd, &mask, &golden_val))
			_golden_setting_add(&_golden, addr, mask, golden_val);
	}
	/* set reg value (mask mode) */
	else if (sscanf(buf, "set 0x%x 0x%x 0x%x", &addr, &mask, &golden_val) == 3)
		_golden_write_reg(addr, mask, golden_val);
	/* set reg value (bit mode) */
	/* XXX: mask is bit number (alias) */
	else if (sscanf(buf, "set 0x%x %d %d", &addr, &mask, &golden_val) == 3) {
		if (mask >= 0 && mask <= 31) {
			golden_val = (golden_val & BIT(0)) << mask;
			mask = BIT(0) << mask;
			_golden_write_reg(addr, mask, golden_val);
		}
	}
	/* set reg value (normal mode) */
	else if (sscanf(buf, "set 0x%x 0x%x", &addr, &golden_val) == 2)
		_golden_write_reg(addr, 0xFFFFFFFF, golden_val);
	/* set to dump pmic reg value */
	else if (sscanf(buf, "set_pmic_manual_dump 0x%x", &addr) == 1) {
		if (_pmic_manual_dump.addr_array) {
			if (_pmic_manual_dump.array_pos < _pmic_manual_dump.array_size) {
				_pmic_manual_dump.addr_array[_pmic_manual_dump.array_pos++] = addr;
				base  = (addr & (~(unsigned long)REMAP_SIZE_MASK));

				if (!_is_pmic_addr(addr) &&
				    !_is_exist_in_phys_to_virt_table(base) &&
				    _base_remap.table) {

					table = _base_remap.table;
					table[_base_remap.table_pos].phys_base = base;
					table[_base_remap.table_pos].virt_base
						= ioremap_nocache(base, REMAP_SIZE_MASK + 1);

					if (!table[_base_remap.table_pos].virt_base)
						pr_info("Power_gs: ioremap_nocache(0x%x, 0x%x) return NULL\n"
							, base, REMAP_SIZE_MASK + 1);

					if (_base_remap.table_pos < _base_remap.table_size)
						_base_remap.table_pos++;
					else
						pr_info("Power_gs: base_remap in maximum size, return for skipped\n");
				}
			} else
				pr_info("Power_gs: pmic_manual_dump array is full\n");
		} else
			pr_info("Power_gs: pmic_manual_dump init fail\n");
	}
	/* XXX: 63 = sizeof(cmd) - 1 */
	else if (sscanf(buf, "%63s", cmd) == 1) {
		if (!strcmp(cmd, "enable"))
			_golden_setting_enable(&_golden);
		else if (!strcmp(cmd, "disable"))
			_golden_setting_disable(&_golden);
		else if (!strcmp(cmd, "normal"))
			_golden_setting_set_mode(&_golden, MODE_NORMAL);
		else if (!strcmp(cmd, "compare"))
			_golden_setting_set_mode(&_golden, MODE_COMPARE);
		else if (!strcmp(cmd, "apply"))
			_golden_setting_set_mode(&_golden, MODE_APPLY);
		else if (!strcmp(cmd, "color"))
			_golden_setting_set_mode(&_golden, MODE_COLOR);
		else if (!strcmp(cmd, "diff"))
			_golden_setting_set_mode(&_golden, MODE_DIFF);
		else if (!strcmp(cmd, "filter"))
			_golden.func[0] = '\0';
		else if (!strcmp(cmd, "dump_suspend"))
			mt_power_gs_suspend_compare(GS_ALL);
		else if (!strcmp(cmd, "dump_dpidle"))
			mt_power_gs_dpidle_compare(GS_ALL);
		else if (!strcmp(cmd, "dump_sodi"))
			mt_power_gs_sodi_compare(GS_ALL);
		else if (!strcmp(cmd, "free_pmic_manual_dump")) {
			if (_pmic_manual_dump.addr_array)
				_pmic_manual_dump.array_pos = 0;
		}
	}

	free_page((size_t)buf);

	return count;
}


#define PROC_FOPS_RW(name)						\
	static int name ## _proc_open(struct inode *inode, struct file *file) \
	{								\
		return single_open_size(file, name ## _proc_show, NULL, 2 * PAGE_SIZE);	\
	}								\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)						\
	static int name ## _proc_open(struct inode *inode, struct file *file) \
	{								\
		return single_open(file, name ## _proc_show, NULL);	\
	}								\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(golden_test);

void __attribute__((weak)) mt_power_gs_internal_init(void)
{
}

static int mt_golden_setting_init(void)
{
#define GOLDEN_SETTING_BUF_SIZE (2 * PAGE_SIZE)

	unsigned int *buf;

	buf = kmalloc(GOLDEN_SETTING_BUF_SIZE, GFP_KERNEL);

	if (buf) {
		_golden_setting_init(&_golden, buf, GOLDEN_SETTING_BUF_SIZE);

#ifdef CONFIG_OF
		_golden.phy_base = 0;
		_golden.io_base = 0;
#endif
		{
			struct proc_dir_entry *dir = NULL;
			int i;

			const struct {
				const char *name;
				const struct file_operations *fops;
			} entries[] = {
				PROC_ENTRY(golden_test),
			};

			dir = proc_mkdir("golden", NULL);

			if (!dir) {
				pr_err("[%s]: fail to mkdir /proc/golden\n", __func__);
				return -ENOMEM;
			}

			for (i = 0; i < ARRAY_SIZE(entries); i++) {
				if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
					pr_err("[%s]: fail to mkdir /proc/golden/%s\n", __func__, entries[i].name);
			}

			_pmic_manual_dump.array_size = REMAP_SIZE_MASK;
			if (!_pmic_manual_dump.addr_array) {
				_pmic_manual_dump.addr_array =
					kmalloc(sizeof(unsigned int) * REMAP_SIZE_MASK + 1, GFP_KERNEL);
				pr_warn("Power_gs: pmic_manual_dump array malloc done\n");
			}

			_base_remap.table_size = REMAP_SIZE_MASK;
			_base_remap.table_pos = 0;
			mt_power_gs_internal_init();
			mt_power_gs_table_init();
		}
	}

	return 0;
}
module_init(mt_golden_setting_init);


unsigned int _golden_read_reg(unsigned int addr)
{
	unsigned int reg_val;
	unsigned int base = addr & (~(unsigned long)REMAP_SIZE_MASK);
	unsigned int offset = addr & (unsigned long)REMAP_SIZE_MASK;
	void __iomem *io_base;

	if (_is_pmic_addr(addr))
		reg_val = gs_pmic_read(addr);
	else {
#if 0
		io_base = ioremap_nocache(base, REMAP_SIZE_MASK+1);
		reg_val = ioread32(io_base + offset);
		iounmap(io_base);
#else
		io_base = _get_virt_base_from_table(base);
		if (io_base)
			reg_val = ioread32(io_base + offset);
		else
			reg_val = 0;
#endif
	}

	return reg_val;
}

void _golden_write_reg(unsigned int addr, unsigned int mask, unsigned int reg_val)
{
	void __iomem *io_addr;

	if (_is_pmic_addr(addr))
		pmic_config_interface(addr, reg_val, mask, 0x0);
	else {
		io_addr = _golden_io_phys_to_virt(addr);
		writel((ioread32(io_addr) & ~mask) | (reg_val & mask), io_addr);
	}
}

/* Check phys addr is existed in table or not */
bool _is_exist_in_phys_to_virt_table(unsigned int phys_base)
{
	unsigned int k;

	if (_base_remap.table)
		for (k = 0; k < _base_remap.table_pos; k++)
			if (_base_remap.table[k].phys_base == phys_base)
				return true;

	return false;
}

void __iomem *_get_virt_base_from_table(unsigned int phys_base)
{
	unsigned int k;
	void __iomem *io_base = 0;

	if (_base_remap.table)
		for (k = 0; k < _base_remap.table_pos; k++)
			if (_base_remap.table[k].phys_base == phys_base)
				return (io_base = _base_remap.table[k].virt_base);

	pr_warn("Power_gs: cannot find virtual address, return value 0\n");
	return io_base;
}

unsigned int mt_power_gs_base_remap_init(char *scenario, char *pmic_name,
			 const unsigned int *pmic_gs, unsigned int pmic_gs_len)
{
	unsigned int i, base;
	struct phys_to_virt_table *table;

	if (!_base_remap.table) {
		_base_remap.table = kmalloc(sizeof(struct phys_to_virt_table) * REMAP_SIZE_MASK + 1, GFP_KERNEL);
		pr_warn("Power_gs: golden setting base_remap table malloc done\n");
	}

	if (_base_remap.table) {
		table = _base_remap.table;

		for (i = 0; i < pmic_gs_len; i += 3) {
			base = (pmic_gs[i] & (~(unsigned long)REMAP_SIZE_MASK));

			if (!_is_exist_in_phys_to_virt_table(base)) {
				table[_base_remap.table_pos].phys_base = base;
				table[_base_remap.table_pos].virt_base = ioremap_nocache(base, REMAP_SIZE_MASK + 1);

				if (!table[_base_remap.table_pos].virt_base) {
					pr_warn("Power_gs: ioremap_nocache(0x%x, 0x%x) return NULL\n"
							, base, REMAP_SIZE_MASK + 1);
				}

				if (_base_remap.table_pos < _base_remap.table_size)
					_base_remap.table_pos++;
				else {
					pr_warn("Power_gs: base_remap in maximum size, return for skipped\n");
					return 0;
				}
			}
		}
	}

	return 0;
}

#define PER_LINE_TO_PRINT 8

void mt_power_gs_pmic_manual_dump(void)
{
	unsigned int i, dump_cnt = 0;
	char *p;

	if (_pmic_manual_dump.addr_array && _pmic_manual_dump.array_pos) {
		p = buf;
		p += snprintf(p, sizeof(buf), "\n");
		p += snprintf(p, sizeof(buf) - (p - buf),
		"Scenario - PMIC - Addr       - Value\n");

		for (i = 0; i < _pmic_manual_dump.array_pos; i++) {
			dump_cnt++;
			p += snprintf(p, sizeof(buf) - (p - buf),
				"Manual   - PMIC - 0x%08x - 0x%08x\n",
				_pmic_manual_dump.addr_array[i],
				_golden_read_reg(_pmic_manual_dump.addr_array[i]));

			if (dump_cnt && ((dump_cnt % PER_LINE_TO_PRINT) == 0)) {
				pr_warn("%s", buf);
				p = buf;
				p += snprintf(p, sizeof(buf), "\n");
			}
		}
		if (dump_cnt % PER_LINE_TO_PRINT)
			pr_warn("%s", buf);
	}
}

void mt_power_gs_compare(char *scenario, char *pmic_name,
			 const unsigned int *pmic_gs, unsigned int pmic_gs_len)
{
	unsigned int i, k, val0, val1, val2, diff, dump_cnt = 0;
	char *p;

	/* dump diff mode */
	if (slp_chk_golden_diff_mode) {
		p = buf;
		p += snprintf(p, sizeof(buf), "\n");
		p += snprintf(p, sizeof(buf) - (p - buf),
		"Scenario - PMIC - Addr       - Value      - Mask       - Golden     - Wrong Bit\n");

		for (i = 0; i < pmic_gs_len; i += 3) {
			val0 = _golden_read_reg(pmic_gs[i]);
			val1 = val0 & pmic_gs[i + 1];
			val2 = pmic_gs[i + 2] & pmic_gs[i + 1];

			if (val1 != val2) {
				dump_cnt++;
				p += snprintf(p, sizeof(buf) - (p - buf),
					"%s - %s - 0x%08x - 0x%08x - 0x%08x - 0x%08x -",
					scenario, pmic_name, pmic_gs[i], val0, pmic_gs[i + 1], pmic_gs[i + 2]);

				for (k = 0, diff = val1 ^ val2; diff != 0; k++, diff >>= 1) {
					if ((diff % 2) != 0)
						p += snprintf(p, sizeof(buf) - (p - buf), " %d", k);
				}

				p += snprintf(p, sizeof(buf) - (p - buf), "\n");

				if (dump_cnt && ((dump_cnt % PER_LINE_TO_PRINT) == 0)) {
					pr_warn("%s", buf);
					p = buf;
					p += snprintf(p, sizeof(buf), "\n");
				}
			}

		}
		if (dump_cnt % PER_LINE_TO_PRINT)
			pr_warn("%s", buf);

	/* dump raw data mode */
	} else {
		p = buf;
		p += snprintf(p, sizeof(buf), "\n");
		p += snprintf(p, sizeof(buf) - (p - buf),
		"Scenario - PMIC - Addr       - Value\n");

		for (i = 0; i < pmic_gs_len; i += 3) {
			val0 = _golden_read_reg(pmic_gs[i]);

			dump_cnt++;
			p += snprintf(p, sizeof(buf) - (p - buf),
				"%s - %s - 0x%08x - 0x%08x\n",
				scenario, pmic_name, pmic_gs[i], val0);

			if (dump_cnt && ((dump_cnt % PER_LINE_TO_PRINT) == 0)) {
				pr_warn("%s", buf);
				p = buf;
				p += snprintf(p, sizeof(buf), "\n");
			}
		}
		if (dump_cnt % PER_LINE_TO_PRINT)
			pr_warn("%s", buf);
	}
	pr_warn("Done...\n");
}
