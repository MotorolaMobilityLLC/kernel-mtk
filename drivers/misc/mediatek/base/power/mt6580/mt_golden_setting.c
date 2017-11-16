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

/**
* @file    mt_golden_setting.c
* @brief   Driver for Golden Setting
*
*/

#define __MT_GOLDEN_SETTING_C__

/*============================================================= */
/* Include files						*/
/*============================================================= */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <asm/io.h>
#include <mach/mt_clkmgr.h>
#include <mt-plat/upmu_common.h>

/*=============================================================	*/
/* Macro definition						*/
/*=============================================================	*/
#ifndef FALSE
	#define FALSE (0)
#endif

#ifndef TRUE
	#define TRUE (1)
#endif

#ifndef NULL
	#define NULL (0)
#endif

/* #define BIT(_bit_)          (unsigned int)(1 << (_bit_))	*/
#define BITS(_bits_, _val_) \
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define BITMASK(_bits_)     (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define USING_XLOG


#define TAG     "Power/clkmgr"
#define HEX_FMT "0x%08x"
#define clk_err(fmt, args...)       \
	pr_err(fmt, ##args)
#define clk_warn(fmt, args...)      \
	pr_warn(fmt, ##args)
#define clk_info(fmt, args...)      \
	pr_info(fmt, ##args)
#define clk_dbg(fmt, args...)       \
	pr_debug(fmt, ##args)

#define FUNC_LV_API		BIT(0)
#define FUNC_LV_LOCKED		BIT(1)
#define FUNC_LV_BODY		BIT(2)
#define FUNC_LV_OP		BIT(3)
#define FUNC_LV_REG_ACCESS	BIT(4)
#define FUNC_LV_DONT_CARE	BIT(5)

#define FUNC_LV_MASK	\
	(FUNC_LV_API | FUNC_LV_LOCKED | FUNC_LV_BODY | FUNC_LV_OP | FUNC_LV_REG_ACCESS | FUNC_LV_DONT_CARE)

/*					*/
/* Register access function		*/
/*					*/

#if defined(CONFIG_CLKMGR_SHOWLOG)

#if defined(CONFIG_CLKMGR_EMULATION)   /* XXX: NOT ACCESS REGISTER	*/

#define clk_readl(addr)									\
	((FUNC_LV_REG_ACCESS & FUNC_LV_MASK) ?						\
	pr_debug("clk_readl("HEX_FMT") @ %s():%d\n",					\
	(addr), __func__, __LINE__) : 0, 0)

#define clk_writel(addr, val)								\
	do {										\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_writel("HEX_FMT", "HEX_FMT") @ %s():%d\n",	\
				(addr), (val), __func__, __LINE__);			\
	} while (0)

#define clk_setl(addr, val)								\
	do {										\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_setl("HEX_FMT", "HEX_FMT") @ %s():%d\n",		\
				(addr), (val), __func__, __LINE__);			\
	} while (0)

#define clk_clrl(addr, val)								\
	do {										\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_clrl("HEX_FMT", "HEX_FMT") @ %s():%d\n",		\
			(addr), (val), __func__, __LINE__);				\
	} while (0)

#else                           /* XXX: ACCESS REGISTER	*/

#define clk_readl(addr)									\
	((FUNC_LV_REG_ACCESS & FUNC_LV_MASK) ?						\
		pr_debug("clk_readl("HEX_FMT") @ %s():%d\n",				\
		(addr), __func__, __LINE__) :						\
		0, DRV_Reg32(addr))

#define clk_writel(addr, val)								\
	do {										\
		unsigned int value;							\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_writel("HEX_FMT", "HEX_FMT") @ %s():%d\n",	\
				(addr), (value = val), __func__, __LINE__);		\
		mt65xx_reg_sync_writel((value), (addr));				\
	} while (0)

#define clk_setl(addr, val)								\
	do {										\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_setl("HEX_FMT", "HEX_FMT") @ %s():%d\n",		\
				(addr), (val), __func__, __LINE__);			\
		mt65xx_reg_sync_writel(clk_readl(addr) | (val), (addr));		\
	} while (0)

#define clk_clrl(addr, val)								\
	do {										\
		if (FUNC_LV_REG_ACCESS & FUNC_LV_MASK)					\
			pr_debug("clk_clrl("HEX_FMT", "HEX_FMT") @ %s():%d\n",		\
				(addr), (val), __func__, __LINE__);			\
		mt65xx_reg_sync_writel(clk_readl(addr) & ~(val), (addr));		\
	} while (0)

#endif /* defined(CONFIG_CLKMGR_EMULATION)	*/

#else

#define clk_readl(addr)         DRV_Reg32(addr)
#define clk_writel(addr, val)   mt65xx_reg_sync_writel(val, addr)
#define clk_setl(addr, val)     mt65xx_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val)     mt65xx_reg_sync_writel(clk_readl(addr) & ~(val), addr)

#endif /* defined(CONFIG_CLKMGR_SHOWLOG)	*/

bool is_already_snap_shot = false;

/*			*/
/* Golden setting	*/
/*			*/
#if defined(CONFIG_MT_ENG_BUILD)

enum print_mode {
	MODE_NORMAL,
	MODE_COMPARE,
	MODE_APPLY,
	MODE_COLOR,
	MODE_DIFF,
};

struct golden_setting {
	unsigned int addr;
	unsigned int mask;
	unsigned int golden_val;
};

struct snapshot {
	const char *func;
	unsigned int line;
	unsigned int reg_val[1]; /* XXX: actually variable length	*/
};

struct golden {
	unsigned int is_golden_log;

	enum print_mode mode;

	char func[64]; /* TODO: check the size is OK or not	*/
	unsigned int line;

	unsigned int *buf;
	unsigned int buf_size;

	struct golden_setting *buf_golden_setting;
	unsigned int nr_golden_setting;
	unsigned int max_nr_golden_setting;

	struct snapshot *buf_snapshot;
	unsigned int max_nr_snapshot;
	unsigned int snapshot_head;
	unsigned int snapshot_tail;
#ifdef CONFIG_OF
	unsigned int phy_base;
	void __iomem *io_base;
#endif
};

#define SIZEOF_SNAPSHOT(g) (sizeof(struct snapshot) + sizeof(unsigned int) * (g->nr_golden_setting - 1))

static struct golden _golden;

static void _golden_setting_enable(struct golden *g)
{
	if (NULL != g) {
		g->buf_snapshot = (struct snapshot *) &(g->buf_golden_setting[g->nr_golden_setting]);
		g->max_nr_snapshot = (g->buf_size - sizeof(struct golden_setting) *
					g->nr_golden_setting) / SIZEOF_SNAPSHOT(g);
		g->snapshot_head = 0;
		g->snapshot_tail = 0; /* TODO: check it	*/

		g->is_golden_log = TRUE;
	}
}

static void _golden_setting_disable(struct golden *g)
{
	if (NULL != g) {
		g->is_golden_log = FALSE;

		g->func[0] = '\0';

		g->buf_golden_setting = (struct golden_setting *)g->buf;
		g->nr_golden_setting = 0;
		g->max_nr_golden_setting = g->buf_size / 3 / sizeof(struct golden_setting);
	}
}

static void _golden_setting_set_mode(struct golden *g, enum print_mode mode)
{
	g->mode = mode;
}

static void _golden_setting_init(struct golden *g, unsigned int *buf, unsigned int buf_size)
{
	if (NULL != g
	    && NULL != buf
	   ) {
		g->mode = MODE_NORMAL;

		g->buf = buf;
		g->buf_size = buf_size;

		_golden_setting_disable(g);
	}
}

static void _golden_setting_add(struct golden *g, unsigned int addr, unsigned int mask, unsigned golden_val)
{
	if (NULL != g
	    && FALSE == g->is_golden_log
	    && g->nr_golden_setting < g->max_nr_golden_setting
	    && mask != 0
	   ) {
		g->buf_golden_setting[g->nr_golden_setting].addr = addr;
		g->buf_golden_setting[g->nr_golden_setting].mask = mask;
		g->buf_golden_setting[g->nr_golden_setting].golden_val = golden_val;

		g->nr_golden_setting++;
	}
}

static bool _is_pmic_addr(unsigned int addr)
{
	return (addr & 0xF0000000) ? FALSE : TRUE;
}

#ifdef CONFIG_OF

#define REMAP_SIZE_MASK     0xFFF

static void __iomem *_golden_io_phys_to_virt(unsigned int addr)
{
	unsigned int base = addr & (~(unsigned long)REMAP_SIZE_MASK);
	unsigned int offset = addr & (unsigned long)REMAP_SIZE_MASK;

	if (!_golden.phy_base || _golden.phy_base != base)
		if (_golden.io_base)
			iounmap(_golden.io_base);

	_golden.phy_base = base;
	_golden.io_base = ioremap_nocache(_golden.phy_base, REMAP_SIZE_MASK+1);

	if (!_golden.io_base)
		clk_warn("warning: ioremap_nocache(0x%x, 0x%x) return NULL\n", base, REMAP_SIZE_MASK+1);

	return (_golden.io_base + offset);
}
#endif

static void _golden_write_reg(unsigned int addr, unsigned int mask, unsigned int reg_val)
{
	clk_dbg("@%s:%d(addr = %X, mask = %X, reg_val = %X)\n", __func__, __LINE__, addr, mask, reg_val);

	if (_is_pmic_addr(addr))
		pmic_config_interface(addr, reg_val, mask, 0x0);
	else {
#ifdef CONFIG_OF
		void __iomem *io_addr = _golden_io_phys_to_virt(addr);

		writel((ioread32(io_addr) & ~mask) | (reg_val & mask), io_addr);
#else
		*((unsigned int *)IO_PHYS_TO_VIRT(addr)) =
			(*((unsigned int *)IO_PHYS_TO_VIRT(addr)) & ~mask) | (reg_val & mask);
#endif
	}
}

static unsigned int _golden_read_reg(unsigned int addr)
{
	unsigned int reg_val;

	if (_is_pmic_addr(addr))
		pmic_read_interface(addr, &reg_val, 0xFFFFFFFF, 0x0);
	else {
#ifdef CONFIG_OF
		reg_val = ioread32(_golden_io_phys_to_virt(addr));
#else
		reg_val = *((unsigned int *)IO_PHYS_TO_VIRT(addr));
#endif
	}

	return reg_val;
}

static int _is_snapshot_full(struct golden *g)
{
	if (g->snapshot_head + 1 == g->snapshot_tail
	    || g->snapshot_head + 1 == g->snapshot_tail + g->max_nr_snapshot
	   )
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
	if (NULL != g
	    && !_is_snapshot_full(g)
	   ) {
		int idx = g->snapshot_head++;

		if (g->snapshot_head == g->max_nr_snapshot)
			g->snapshot_head = 0;

		return (struct snapshot *)((int)(g->buf_snapshot) + SIZEOF_SNAPSHOT(g) * idx);
	} else
		return NULL;
}

static struct snapshot *_snapshot_consume(struct golden *g)
{
	if (NULL != g
		&& !_is_snapshot_empty(g)
	   ) {
		int idx = g->snapshot_tail++;

		if (g->snapshot_tail == g->max_nr_snapshot)
			g->snapshot_tail = 0;

		is_already_snap_shot = false;

		return (struct snapshot *)((int)(g->buf_snapshot) + SIZEOF_SNAPSHOT(g) * idx);
	} else
		return NULL;
}

static int _snapshot_golden_setting(struct golden *g, const char *func, const unsigned int line)
{
	struct snapshot *snapshot;
	int i;

	snapshot = _snapshot_produce(g);
	if (NULL != g
	    && TRUE == g->is_golden_log
	    && (g->func[0] == '\0' || (!strcmp(g->func, func) && ((g->line == line) || (g->line == 0))))
	    && NULL != snapshot) {
		snapshot->func = func;
		snapshot->line = line;

		for (i = 0; i < g->nr_golden_setting; i++) {
			if (MODE_APPLY == _golden.mode) {
				_golden_write_reg(g->buf_golden_setting[i].addr,
						  g->buf_golden_setting[i].mask,
						  g->buf_golden_setting[i].golden_val
						 );
			}

			snapshot->reg_val[i] = _golden_read_reg(g->buf_golden_setting[i].addr);
		}

		is_already_snap_shot = true;

		return 0;
	}
	return -1;
}

#endif /* CONFIG_MT_ENG_BUILD */

int snapshot_golden_setting(const char *func, const unsigned int line)
{
#if defined(CONFIG_MT_ENG_BUILD)
	return _snapshot_golden_setting(&_golden, func, line);
#else
	return -1;
#endif
}
EXPORT_SYMBOL(snapshot_golden_setting);

#if defined(CONFIG_MT_ENG_BUILD)

static int _parse_mask_val(char *buf, unsigned int *mask, unsigned int *golden_val)
{
	unsigned int i, bit_shift;
	unsigned int mask_result;
	unsigned int golden_val_result;

	for (i = 0,
	     bit_shift = 1 << 31,
	     mask_result = 0,
	     golden_val_result = 0;
	     bit_shift > 0;
	    ) {
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
	static char _mask_str[64];
	char mask_str[] = "0bxxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx";
	unsigned int i, bit_shift;

	strncpy(_mask_str, mask_str, sizeof(_mask_str));
	for (i = 2,
	     bit_shift = 1 << 31;
	     bit_shift > 0;
	    ) {
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
	static char _diff_str[64];
	char diff_str[] = "0b    _    _    _    _    _    _    _    ";
	unsigned int i, bit_shift;

	strncpy(_diff_str, diff_str, sizeof(_diff_str));
	for (i = 2,
	     bit_shift = 1 << 31;
	     bit_shift > 0;
	    ) {
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
	static char _clr_str[512];
	char clr_str[] = "0b"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX"_"XXXX;
	unsigned int i, bit_shift;

	strncpy(_clr_str, clr_str, sizeof(_clr_str));
	for (i = 2,
	     bit_shift = 1 << 31;
	     bit_shift > 0;
	    ) {
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

			i += strlen(EC) + strlen(FC); /* XXX: -1 is for '\0' (sizeof)	*/

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
	static int buf_golden_setting_idx;
	int i = 0;

	buf_golden_setting_idx = 0;

	if (FALSE == _golden.is_golden_log) {
		for (i = 0; i < _golden.nr_golden_setting; i++)
			seq_printf(m, ""HEX_FMT" "HEX_FMT" "HEX_FMT"\n",
			_golden.buf_golden_setting[i].addr,
			_golden.buf_golden_setting[i].mask,
			_golden.buf_golden_setting[i].golden_val);
	}

	if (0 == _golden.nr_golden_setting) {
		seq_puts(m, "\n********** golden_test help *********\n");
		seq_puts(m, "1.   disable snapshot:                  ");
		seq_puts(m, "echo disable > /proc/clkmgr/golden_test\n");
		seq_puts(m, "2.   insert golden setting (tool mode): ");
		seq_puts(m, "echo 0x10000000 (addr) 0bxxxx_xxxx_xxxx_xxxx_0001_0100_1001_0100 (mask & golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(2.) insert golden setting (hex mode):  ");
		seq_puts(m, "echo 0x10000000 (addr) 0xFFFF (mask) 0x1494 (golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(2.) insert golden setting (dec mode):  ");
		seq_puts(m, "echo 268435456 (addr) 65535 (mask) 5268 (golden value) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "3.   set filter:                        ");
		seq_puts(m, "echo filter func_name [line_num] > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(3.) disable filter:                    ");
		seq_puts(m, "echo filter > /proc/clkmgr/golden_test\n");
		seq_puts(m, "4.   enable snapshot:                   ");
		seq_puts(m, "echo enable > /proc/clkmgr/golden_test\n");
		seq_puts(m, "5.   set compare mode:                  ");
		seq_puts(m, "echo compare > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set apply mode:                    ");
		seq_puts(m, "echo apply > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set color mode:                    ");
		seq_puts(m, "echo color > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) set diff mode:                     ");
		seq_puts(m, "echo color > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(5.) disable compare/apply/color mode:  ");
		seq_puts(m, "echo normal > /proc/clkmgr/golden_test\n");
		seq_puts(m, "6.   set register value (normal mode):  ");
		seq_puts(m, "echo set 0x10000000 (addr) 0x13201494 (reg val) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(6.) set register value (mask mode):    ");
		seq_puts(m, "echo set 0x10000000 (addr) 0xffff (mask) 0x13201494 (reg val) > /proc/clkmgr/golden_test\n");
		seq_puts(m, "(6.) set register value (bit mode):     ");
		seq_puts(m, "echo set 0x10000000 (addr) 0 (bit num) 1 (reg val) > /proc/clkmgr/golden_test\n");
	} else {
		static struct snapshot *snapshot;

		if (0 == 0 /* off    buf_golden_setting_idx	*/
		    && !strcmp(_golden.func, __func__) && (_golden.line == 0)) {
			snapshot_golden_setting(__func__, 0);
		}
		while ((0 != buf_golden_setting_idx) || (NULL != (snapshot = _snapshot_consume(&_golden)))) {
			if (0 == buf_golden_setting_idx) {
				seq_printf(m, "// @ %s():%d\n", snapshot->func, snapshot->line);
				clk_dbg("// @ %s():%d\n", snapshot->func, snapshot->line);
			}
			for (i = buf_golden_setting_idx,
				buf_golden_setting_idx = 0;
				i < _golden.nr_golden_setting; i++) {
				if (MODE_NORMAL == _golden.mode	||
					((_golden.buf_golden_setting[i].mask &
					_golden.buf_golden_setting[i].golden_val) !=
					(_golden.buf_golden_setting[i].mask &
					snapshot->reg_val[i]))) {
					if (MODE_COLOR == _golden.mode) {
							seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
								   _golden.buf_golden_setting[i].addr,
								   _golden.buf_golden_setting[i].mask,
								   snapshot->reg_val[i],
								   _gen_color_str(_golden.buf_golden_setting[i].mask,
								   _golden.buf_golden_setting[i].golden_val,
								   snapshot->reg_val[i]));
							clk_dbg(HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
								   _golden.buf_golden_setting[i].addr,
								   _golden.buf_golden_setting[i].mask,
								   snapshot->reg_val[i],
								   _gen_color_str(_golden.buf_golden_setting[i].mask,
								   _golden.buf_golden_setting[i].golden_val,
								   snapshot->reg_val[i]));
					} else if (MODE_DIFF == _golden.mode) {
							seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
								   _golden.buf_golden_setting[i].addr,
								   _golden.buf_golden_setting[i].mask,
								   snapshot->reg_val[i],
								   _gen_mask_str(_golden.buf_golden_setting[i].mask,
								   snapshot->reg_val[i]));
							clk_dbg(HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
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
							clk_dbg(HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\t%s\n",
								_golden.buf_golden_setting[i].addr,
								_golden.buf_golden_setting[i].mask,
								_golden.buf_golden_setting[i].golden_val,
								_gen_diff_str(_golden.buf_golden_setting[i].mask,
								_golden.buf_golden_setting[i].golden_val,
								snapshot->reg_val[i]));
					} else {
							seq_printf(m, HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\n",
								_golden.buf_golden_setting[i].addr,
								_golden.buf_golden_setting[i].mask,
								snapshot->reg_val[i]);
							clk_dbg(HEX_FMT"\t"HEX_FMT"\t"HEX_FMT"\n",
								_golden.buf_golden_setting[i].addr,
								_golden.buf_golden_setting[i].mask,
								snapshot->reg_val[i]);
						}
					}
					if (0) { /* ((p - start_p) + (p - page) >= PAGE_SIZE) {	*/
						buf_golden_setting_idx = i + 1;
						break;
					}
				}
				if (0) /* ((p - start_p) + (p - page) >= PAGE_SIZE)	*/
					break;
			}
		}
	return 0; /* len < count ? len : count;	*/
}

static int golden_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	char cmd[64];
	unsigned int addr;
	unsigned int mask;
	unsigned int golden_val;

	/* set golden setting (hex mode)	*/
	if (sscanf(buf, "0x%x 0x%x 0x%x", &addr, &mask, &golden_val) == 3)
		_golden_setting_add(&_golden, addr, mask, golden_val);
	/* set golden setting (dec mode)	*/
	else if (sscanf(buf, "%d %d %d", &addr, &mask, &golden_val) == 3)
		_golden_setting_add(&_golden, addr, mask, golden_val);
	/* set filter (func + line)	*/
	/* else if (sscanf(buf, "filter %63s %d", _golden.func, &_golden.line) == 2) */
		/* XXX: 63 = sizeof(_golden.func) - 1	*/
	/* set filter (func)	*/
	else if (sscanf(buf, "filter %63s", _golden.func) == 1) /* XXX: 63 = sizeof(_golden.func) - 1	*/
		_golden.line = 0;
	/* set golden setting (mixed mode)	*/
	else if (sscanf(buf, "0x%x 0b%63s", &addr, cmd) == 2) { /* XXX: 63 = sizeof(cmd) - 1	*/
		if (!_parse_mask_val(cmd, &mask, &golden_val))
			_golden_setting_add(&_golden, addr, mask, golden_val);
	}
	/* set reg value (mask mode)	*/
	else if (sscanf(buf, "set 0x%x 0x%x 0x%x", &addr, &mask, &golden_val) == 3)
		_golden_write_reg(addr, mask, golden_val);
	/* set reg value (bit mode)	*/
	else if (sscanf(buf, "set 0x%x %d %d", &addr, &mask, &golden_val) == 3) {
		if (0 <= mask && mask <= 31) { /* XXX: mask is bit number (alias) */
			golden_val = (golden_val & BIT(0)) << mask;
			mask = BIT(0) << mask;
			_golden_write_reg(addr, mask, golden_val);
		}
	}
	/* set reg value (normal mode)	*/
	else if (sscanf(buf, "set 0x%x 0x%x", &addr, &golden_val) == 2)
		_golden_write_reg(addr, 0xFFFFFFFF, golden_val);
	else if (sscanf(buf, "%63s", cmd) == 1) { /* XXX: 63 = sizeof(cmd) - 1	*/
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
	}

	free_page((unsigned int)buf);
	return count;
}


#define PROC_FOPS_RW(name)							\
	static int name ## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open_size(file, name ## _proc_show, NULL, 2 * PAGE_SIZE);		\
	}									\
	static const struct file_operations name ## _proc_fops = {		\
		.owner          = THIS_MODULE,					\
		.open           = name ## _proc_open,				\
		.read           = seq_read,					\
		.llseek         = seq_lseek,					\
		.release        = single_release,				\
		.write          = name ## _proc_write,				\
	}

#define PROC_FOPS_RO(name)							\
	static int name ## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, NULL);		\
	}									\
	static const struct file_operations name ## _proc_fops = {		\
		.owner          = THIS_MODULE,					\
		.open           = name ## _proc_open,				\
		.read           = seq_read,					\
		.llseek         = seq_lseek,					\
		.release        = single_release,				\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(golden_test);

#endif /* CONFIG_MT_ENG_BUILD */

static int mt_golden_setting_init(void)
{
#if defined(CONFIG_MT_ENG_BUILD)
#define GOLDEN_SETTING_BUF_SIZE (2 * PAGE_SIZE)

	unsigned int *buf;

	buf = kmalloc(GOLDEN_SETTING_BUF_SIZE, GFP_KERNEL);

	if (NULL != buf) {
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
				clk_err("[%s]: fail to mkdir /proc/golden\n", __func__);
				return -ENOMEM;
			}

			for (i = 0; i < ARRAY_SIZE(entries); i++) {
				if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
					clk_err("[%s]: fail to mkdir /proc/golden/%s\n", __func__, entries[i].name);
			}
		}
	}

#endif /* CONFIG_MT_ENG_BUILD	*/
	return 0;
}
module_init(mt_golden_setting_init);
