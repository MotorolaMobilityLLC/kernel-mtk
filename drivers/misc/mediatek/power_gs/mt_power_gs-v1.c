#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>

#include "mt_power_gs_array.h"

#define gs_read(addr) (*(volatile u32 *)(addr))

struct proc_dir_entry *mt_power_gs_dir = NULL;

#define DEBUG_BUF_SIZE 200
static char dbg_buf[DEBUG_BUF_SIZE] = { 0 };

static u16 gs_pmic_read(u16 reg)
{
	u32 ret = 0;
	u32 reg_val = 0;

	ret = pmic_read_interface_nolock(reg, &reg_val, 0xFFFF, 0x0);

	return (u16)reg_val;
}

static void mt_power_gs_compare(char *scenario, char *pmic_name,
				const unsigned int *pmic_gs, unsigned int pmic_gs_len)
{
	unsigned int i, k, val1, val2, diff;
	char *p;

	pr_warn("Scenario - PMIC - Addr  - Value  - Mask   - Golden - Wrong Bit\n");

	for (i = 0; i < pmic_gs_len; i += 3) {
		aee_sram_printk("%d\n", i);
		val1 = gs_pmic_read(pmic_gs[i]) & pmic_gs[i + 1];
		val2 = pmic_gs[i + 2] & pmic_gs[i + 1];

		if (val1 != val2) {
			p = dbg_buf;
			p += sprintf(p, "%s - %s - 0x%x - 0x%04x - 0x%04x - 0x%04x -",
				     scenario, pmic_name, pmic_gs[i], gs_pmic_read(pmic_gs[i]),
				     pmic_gs[i + 1], pmic_gs[i + 2]);

			for (k = 0, diff = val1 ^ val2; diff != 0; k++, diff >>= 1) {
				if ((diff % 2) != 0)
					p += sprintf(p, " %d", k);
			}
			pr_warn("%s\n", dbg_buf);
		}
	}
}

void mt_power_gs_dump_suspend(void)
{
#ifdef CONFIG_ARCH_MT6580
	mt_power_gs_compare("Suspend ", "6325",
			    MT6325_PMIC_REG_gs_flightmode_suspend_mode,
			    MT6325_PMIC_REG_gs_flightmode_suspend_mode_len);
#elif defined CONFIG_ARCH_MT6735 || defined CONFIG_ARCH_MT6735M || defined CONFIG_ARCH_MT6753
	mt_power_gs_compare("Suspend ", "6328",
			    MT6328_PMIC_REG_gs_flightmode_suspend_mode,
			    MT6328_PMIC_REG_gs_flightmode_suspend_mode_len);
#elif defined CONFIG_ARCH_MT6755 || defined CONFIG_ARCH_MT6797
	mt_power_gs_compare("Suspend ", "6351",
			    MT6351_PMIC_REG_gs_flightmode_suspend_mode,
			    MT6351_PMIC_REG_gs_flightmode_suspend_mode_len);
#endif
}
EXPORT_SYMBOL(mt_power_gs_dump_suspend);

void mt_power_gs_dump_dpidle(void)
{
#if defined CONFIG_ARCH_MT6755 || defined CONFIG_ARCH_MT6797
	mt_power_gs_compare("DPIdle  ", "6351",
			    MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode,
			    MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode_len);
#endif
}
EXPORT_SYMBOL(mt_power_gs_dump_dpidle);

static void __exit mt_power_gs_exit(void)
{
}

static int __init mt_power_gs_init(void)
{
	mt_power_gs_dir = proc_mkdir("mt_power_gs", NULL);

	if (!mt_power_gs_dir)
		pr_err("[%s]: mkdir /proc/mt_power_gs failed\n", __func__);

	return 0;
}

module_init(mt_power_gs_init);
module_exit(mt_power_gs_exit);

MODULE_DESCRIPTION("MT Low Power Golden Setting");

