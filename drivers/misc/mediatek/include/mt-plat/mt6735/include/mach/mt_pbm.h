#ifndef _MT_PBM_
#define _MT_PBM_

/* #include <cust_pmic.h> */
#include <mach/mt_pmic.h>

#ifdef DISABLE_DLPT_FEATURE
#define DISABLE_PBM_FEATURE
#endif

struct pbm {
	u8 feature_en;
	u8 pbm_drv_done;
	u32 hpf_en;
};

struct hpf {
	bool switch_md1;
	bool switch_md2;
	bool switch_gpu;
	bool switch_flash;

	int cpu_volt;
	int gpu_volt;
	int cpu_num;

	unsigned long loading_leakage;
	unsigned long loading_dlpt;
	unsigned long loading_md;
	unsigned long loading_cpu;
	unsigned long loading_gpu;
	unsigned long loading_flash;
};

struct mrp {
	bool idMD;
	bool switch_md;
	bool switch_gpu;
	bool switch_flash;

	int cpu_volt;
	int gpu_volt;
	int cpu_num;

	unsigned long loading_dlpt;
	unsigned long loading_cpu;
	unsigned long loading_gpu;
};

enum pbm_kicker {
	KR_DLPT,		/* 0 */
	KR_MD,			/* 1 */
	KR_CPU,			/* 2 */
	KR_GPU,			/* 3 */
	KR_FLASH		/* 4 */
};

enum md_id {
	MD1,
	MD2
};

extern void kicker_pbm_by_dlpt(unsigned int i_max);
extern void kicker_pbm_by_md(enum md_id id, bool status);
extern void kicker_pbm_by_cpu(unsigned int loading, int core, int voltage);
extern void kicker_pbm_by_gpu(bool status, unsigned int loading, int voltage);
extern void kicker_pbm_by_flash(bool status);

#ifndef DISABLE_PBM_FEATURE
extern int g_dlpt_stop;
#endif

#endif
