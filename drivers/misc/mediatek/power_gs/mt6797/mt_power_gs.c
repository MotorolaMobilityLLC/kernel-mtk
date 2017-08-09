#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mt-plat/aee.h>

#include <mt-plat/mt_typedefs.h>
#include <mt_spm_idle.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_power_gs.h>

#include <mach/mt_pmic_wrap.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_hw.h>

#define gs_read(addr) (*(volatile u32 *)(addr))

struct proc_dir_entry *mt_power_gs_dir = NULL;

static kal_uint16 gs6351_pmic_read(kal_uint16 reg)
{
	kal_uint32 ret = 0;
	kal_uint32 reg_val = 0;

	ret = pmic_read_interface(reg, &reg_val, 0xFFFF, 0x0);

	return (kal_uint16)reg_val;
}
/*
static void mt_power_gs_compare_pll(void)
{
	int i;
	#define ID_NAME(n)	{n, __stringify(n)}

	struct {
		const int id;
		const char *name;
	} plls[NR_PLLS] = {
		ID_NAME(ARMCA15PLL),
		ID_NAME(ARMCA7PLL),
		ID_NAME(MAINPLL),
		ID_NAME(MSDCPLL),
		ID_NAME(UNIVPLL),
		ID_NAME(MMPLL),
		ID_NAME(VENCPLL),
		ID_NAME(TVDPLL),
		ID_NAME(MPLL),
		ID_NAME(VCODECPLL),
		ID_NAME(APLL1),
		ID_NAME(APLL2),
	};

	struct {
		const int id;
		const char *name;
	} subsyss[NR_SYSS] = {
		ID_NAME(SYS_MD1),
		ID_NAME(SYS_DIS),
//		ID_NAME(SYS_MFG_ASYNC),
//		ID_NAME(SYS_MFG_2D),
		ID_NAME(SYS_MFG),
		ID_NAME(SYS_ISP),
		ID_NAME(SYS_VDE),
		ID_NAME(SYS_MJC),
		ID_NAME(SYS_VEN),
		ID_NAME(SYS_AUD),
	};

	for (i = 0; i < NR_PLLS; i++) {
		if (pll_is_on(i))
			pr_warn("%s: on\n", plls[i].name);
	}

	for (i = 0; i < NR_SYSS; i++) {
		if (subsys_is_on(i))
			pr_warn("%s: on\n", subsyss[i].name);
	}
}
*/
void mt_power_gs_diff_output(unsigned int val1, unsigned int val2)
{
	int i = 0;
	unsigned int diff = val1 ^ val2;

	while (diff != 0) {
		if ((diff % 2) != 0)
			pr_warn("%d ", i);

		diff /= 2;
		i++;
	}

	pr_warn("\n");
}

void mt_power_gs_compare(char *scenario, const unsigned int *pmic_6351_gs, unsigned int pmic_6351_gs_len)
{
	unsigned int i, val1, val2;

	/*6351*/
	for (i = 0; i < pmic_6351_gs_len; i += 3) {
		aee_sram_printk("%d\n", i);
		val1 = gs6351_pmic_read(pmic_6351_gs[i]) & pmic_6351_gs[i + 1];
		val2 = pmic_6351_gs[i + 2] & pmic_6351_gs[i + 1];

		if (val1 != val2) {
			pr_warn("%s - 6351 - 0x%x - 0x%x - 0x%x - 0x%x - ",
				scenario, pmic_6351_gs[i], gs6351_pmic_read(pmic_6351_gs[i]),
				pmic_6351_gs[i + 1], pmic_6351_gs[i + 2]);
			mt_power_gs_diff_output(val1, val2);
		}
	}

	/*mt_power_gs_compare_pll();*/
}
EXPORT_SYMBOL(mt_power_gs_compare);

static void __exit mt_power_gs_exit(void)
{
	/*return 0;*/
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
