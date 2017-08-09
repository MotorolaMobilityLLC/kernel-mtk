#define	__MT_IDVFS_C__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mach/irqs.h>

/* local includes */
#include "../../../power/mt6797/da9214.h"
#include "mt_idvfs.h"
#include "mt_otp.h"

/* IDVFS ADDR */ /*	TODO: include other	head file */
#ifdef CONFIG_OF
void __iomem				*idvfs_base;			/* (0x10220000) */
static int					idvfs_irq_number1;		/* 331 */
/* define IDVFS_BASE_ADDR	   ((unsigned long)idvfs_base) */
#else
#include "mach/mt_reg_base.h"
/* (0x10222000) */
/* #define	IDVFS_BASE_ADDR		IDVFS_BASEADDR */
#endif

static unsigned int func_lv_mask_idvfs;

/*
 * LOG
 */
#ifdef __KERNEL__
#define	IDVFS_TAG	  "[mt_idvfs] "
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define	idvfs_emerg(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_alert(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_crit(fmt,	args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_error(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_warning(fmt, args...)	  pr_warn(ANDROID_LOG_WARN,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_notice(fmt, args...)	  pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_info(fmt,	args...)	  pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_debug(fmt, args...)	  pr_debug(ANDROID_LOG_DEBUG, IDVFS_TAG, fmt, ##args)
	#define idvfs_ver(fmt, args...)	\
		do {	\
			if (func_lv_mask_idvfs)	\
				pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args);	\
		} while (0)
#else
	#define	idvfs_emerg(fmt, args...)	  pr_emerg(IDVFS_TAG fmt, ##args)
	#define	idvfs_alert(fmt, args...)	  pr_alert(IDVFS_TAG fmt, ##args)
	#define	idvfs_crit(fmt,	args...)	  pr_crit(IDVFS_TAG	fmt, ##args)
	#define	idvfs_error(fmt, args...)	  pr_err(IDVFS_TAG fmt,	##args)
	#define	idvfs_warning(fmt, args...)	  pr_warn(IDVFS_TAG	fmt, ##args)
	#define	idvfs_notice(fmt, args...)	  pr_notice(IDVFS_TAG fmt, ##args)
	#define	idvfs_info(fmt,	args...)	  pr_info(IDVFS_TAG	fmt, ##args)
	#define	idvfs_debug(fmt, args...)	  pr_debug(IDVFS_TAG fmt, ##args)
	#define idvfs_ver(fmt, args...)	\
		do {	\
			if (func_lv_mask_idvfs)	\
				pr_info(IDVFS_TAG	fmt, ##args);	\
		} while (0)
#endif

	/* For Interrupt use	*/
	#if	0 /*EN_ISR_LOG*/
		#define	idvfs_isr_info(fmt,	args...)  idvfs_debug(fmt, ##args)
	#else
		#define	idvfs_isr_info(fmt,	args...)
	#endif
#endif

#define	DA9214_MV_TO_STEP(volt_mv)			((volt_mv -	300) / 10)
#define	DA9214_STEP_TO_MV(step)				(((step	& 0x7f)	* 10) +	300)

/************* for ptp1	temp *************/
#if 1
/* #include	"mach/mt_defptp.h" */
/* static DEFINE_SPINLOCK(idvfs_spinlock); */

#define	NR_FREQ								16

#define	eem_read(addr) \
			idvfs_read(addr)
#define	eem_write(addr,	val) \
			idvfs_write(addr, val)
#define	eem_write_field(addr, range, val) \
			eem_write(addr,	(eem_read(addr)	& ~BITMASK_IDVFS(range)) | BITS_IDVFS(range, val))

#define	DETWINDOW_VAL		0x514

/* EEM */
#define	EEM_BASE			(30000)
#define	EEM_STEP			(1000)

#define	VOLT_2_EEM(volt)	(((volt) - EEM_BASE	+ EEM_STEP - 1)	/ EEM_STEP)
#define	EEM_2_VOLT(value)	(((value) *	EEM_STEP) +	EEM_BASE)

/* SRAM	*/
#define	SRAM_BASE			(90000)
#define	SRAM_STEP			(2500)
/* #define PMIC_2_RMIC(value)	((((value) * EEM_STEP) + EEM_BASE -	SRAM_BASE +	SRAM_STEP -1) /	SRAM_STEP) */

/* CPU */
#define	CPU_PMIC_BASE		(30000)
#define	CPU_PMIC_STEP		(1000)

/* GPU */
#define	GPU_PMIC_BASE		(60300)
#define	GPU_PMIC_STEP		(1282)

#define	VMAX_VAL_BIG		VOLT_2_EEM(118000)
#define	VMIN_VAL_BIG		VOLT_2_EEM(82500)
#define	VMAX_SRAM			VOLT_2_EEM(120000)
#define	VMIN_SRAM			VOLT_2_EEM(90000)
#define	VMAX_VAL_GPU		VOLT_2_EEM(112890)
#define	VMIN_VAL_GPU		VOLT_2_EEM(80820)
#define	VMAX_VAL_LITTLE		VOLT_2_EEM(120000)
#define	VMIN_VAL_LITTLE		VOLT_2_EEM(77500)

#define	DTHI_VAL			0x01		/* positive	*/
#define	DTLO_VAL			0xfe		/* negative	(2's compliment) */
#define	DETMAX_VAL			0xffff		/* This	timeout	value is in	cycles of bclk_ck. */
#define	AGECONFIG_VAL		0x555555
#define	AGEM_VAL			0x0
#define	DVTFIXED_VAL		0x4
#define	VCO_VAL				0x0C
#define	VCO_VAL_BIG			0x14
#define	VCO_VAL_GPU			0x12
#define	DCCONFIG_VAL		0x555555

enum eem_phase {
	EEM_PHASE_INIT01,
	EEM_PHASE_INIT02,
	EEM_PHASE_MON,

	NR_EEM_PHASE,
};

struct eem_devinfo {
	/* M_HW_RES0 0x10206960	*/
	unsigned int BIG_BDES:8;
	unsigned int BIG_MDES:8;
	unsigned int BIG_DCBDET:8;
	unsigned int BIG_DCMDET:8;

	/* M_HW_RES1 0x10206964	*/
	unsigned int BIG_INITEN:1;
	unsigned int BIG_MONEN:1;
	unsigned int BIG_DVFS_L:2;
	unsigned int BIG_TURBO:1;
	unsigned int BIG_BIN_SPEC:3;
	unsigned int BIG_LEAKAGE:8;
	unsigned int BIG_MTDES:8;
	unsigned int BIG_AGEDELTA:8;

};

struct eem_det {
	const char *name;
	struct eem_det_ops *ops;
	int	status;			/* TODO: enable/disable	*/
	int	features;		/* enum	eem_features */
	/* enum	eem_ctrl_id	ctrl_id; */

	/* devinfo */
	unsigned int EEMINITEN;
	unsigned int EEMMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant	*/
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	/* Generated by	EEM	init01.	Used in	EEM	init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for PMIC	*/
	unsigned int pmic_base;
	unsigned int pmic_step;

	/* for debug */
	unsigned int dcvalues[NR_EEM_PHASE];

	unsigned int freqpct30[NR_EEM_PHASE];
	unsigned int eem_26c[NR_EEM_PHASE];
	unsigned int vop30[NR_EEM_PHASE];
	unsigned int eem_eemEn[NR_EEM_PHASE];
	u32	*recordRef;
#if	0
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_EEM_PHASE];
#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;
	unsigned int t250;
	/* dvfs	*/
	unsigned int num_freq_tbl; /* could	be got @ the same time
					  with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency	used to
					  calculate	percentage */
	unsigned char freq_tbl[NR_FREQ]; /*	percentage to maximum freq */

	unsigned int volt_tbl[NR_FREQ];	/* pmic	value */
	unsigned int volt_tbl_init2[NR_FREQ]; /* pmic value	*/
	unsigned int volt_tbl_pmic[NR_FREQ]; /*	pmic value */
	unsigned int volt_tbl_bin[NR_FREQ];	/* pmic	value */
	int	volt_offset;

	unsigned int disabled; /* Disabled by error	or sysfs */
};

static struct eem_devinfo eem_devinfo_data;

/* static struct eem_det eem_detectors[NR_EEM_DET] = { */
static struct eem_det eem_detectors	= {
		.name		= __stringify(EEM_DET_BIG),
		.ops		= 0, /*	&big_det_ops, */
		/* .ctrl_id	= EEM_CTRL_BIG,	*/
		.features	= 0, /*	FEA_INIT01 | FEA_INIT02	| FEA_MON, */
		.max_freq_khz	= 2496000,/* 2496Mhz */
		.VBOOT		= 0x46,	/* VOLT_2_EEM(100000), 1.0v: 0x30, DA9214 =	70*10 +	300	= 1000mv */
		.volt_offset = 0,
		.pmic_base	= 30000, /*	300mv_x100 */
		.pmic_step	= 1000,	 /*	10mv_x100 */
};

void get_devinfo_tmp(struct	eem_devinfo	*p)
{
	int	*val = (int	*)p;

	/* test	pattern	*/
	val[0] = 0x0A14345A; /*	0x10A1590D;	*/
	val[1] = 0x00430023; /*	0x00270023;	*/

}

static void	base_ops_set_phase_tmp(struct eem_det *det)
{
	/* spinlock(); */
	/* spin_lock_irqsave(&idvfs_spinlock, &flags); */

	/* switch bank to 0	*/
	/* eem_write_field(EEMCORESEL, APBSEL, 0); */
	/* base	0x1100B000,	switch bank	0; */

	eem_write(0x1100b400, 0x003f0000);

	/* config EEM register */
	eem_write(0x1100b200, ((det->BDES << 8)	& 0xff00) |	(det->MDES & 0xff));
	eem_write(0x1100b204, (((det->VCO << 16) & 0xff0000) |	((det->MTDES <<	8) & 0xff00) | (det->DVTFIXED &	0xff)));
	eem_write(0x1100b208, ((det->DCBDET	<< 8) &	0xff00)	| (det->DCMDET & 0xff));
	eem_write(0x1100b20c, ((det->AGEDELTA << 8)	& 0xff00) |	(det->AGEM & 0xff));
	eem_write(0x1100b210, det->DCCONFIG);
	eem_write(0x1100b214, det->AGECONFIG);

	/* if (det->AGEM ==	0x0) */
	eem_write(0x1100b234, 0x80000000);

	eem_write(0x1100b220,
		((det->VMAX	<< 24) & 0xff000000)	|
		((det->VMIN	<< 16) & 0xff0000)		|
		((det->DTHI	<< 8)	& 0xff00)		|
		(det->DTLO & 0xff));

	/* eem_write(LIMITVALS,	0xFF0001FE); */
	eem_write(0x1100b224, (((det->VBOOT) & 0xff)));
	eem_write(0x1100b228, (((det->DETWINDOW) & 0xffff)));
	eem_write(0x1100b22c, (((det->DETMAX) &	0xffff)));

	/* clear all pending EEM interrupt & config	EEMINTEN */
	eem_write(0x1100b254, 0xffffffff);
	eem_write(0x1100b25c, 0x00005f01);
	/* AVG and DCV all 0 */
	eem_write(0x1100b23c, ((det->AGEVOFFSETIN << 16) & 0xffff0000) | (det->DCVOFFSETIN & 0xffff));

	/* add */
	eem_write(0x1100b240, 0x0);
	eem_write(0x1100b244, 0x0);
	/* enable EEM INIT measurement */
	eem_write(0x1100b238, 0x00000005);

	/* spinunlock(); */
	/* spin_unlock_irqrestore(&idvfs_spinlock, &flags);	*/
}

static void	eem_init_det_tmp(void) /* struct eem_det *det, struct eem_devinfo *devinfo)	*/
{
	struct eem_det *det	= &eem_detectors;
	struct eem_devinfo *devinfo	= &eem_devinfo_data;

	get_devinfo_tmp(devinfo);

	/* init	with devinfo, init with	constant */
	det->DETWINDOW = DETWINDOW_VAL;
	det->VBOOT = VOLT_2_EEM(100000);

	det->DTHI =	DTHI_VAL;
	det->DTLO =	DTLO_VAL;
	det->DETMAX	= DETMAX_VAL;

	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM =	AGEM_VAL;
	det->DVTFIXED =	DVTFIXED_VAL;
	det->VCO = VCO_VAL;
	det->DCCONFIG =	DCCONFIG_VAL;

	det->MDES	= devinfo->BIG_MDES;
	det->BDES	= devinfo->BIG_BDES;
	det->DCMDET	= devinfo->BIG_DCMDET;
	det->DCBDET	= devinfo->BIG_DCBDET;
	det->EEMINITEN	= devinfo->BIG_INITEN;
	det->EEMMONEN	= devinfo->BIG_MONEN;
	det->VMAX	= VMAX_VAL_BIG;
	det->VMIN	= VMIN_VAL_BIG;
	det->VCO	= VCO_VAL_BIG;

	det->AGEDELTA	= devinfo->BIG_AGEDELTA;
	det->MTDES	= devinfo->BIG_MTDES;

	/* zzzzzz */
	det->DCVOFFSETIN = 0;
	det->AGEVOFFSETIN =	0;

	base_ops_set_phase_tmp(det);

	/* switch bank to 0	*/
	/* eem_write_field(EEMCORESEL, APBSEL, 0); */
	eem_write(0x1100b400, 0x003f0000);							/* switch bank 0; */
	idvfs_ver("DESCHAR	  =	0x%x.\n", eem_read(0x1100b200));	/* base	0x1100B000 */
	idvfs_ver("TEMPCHAR  =	0x%x.\n", eem_read(0x1100b204));
	idvfs_ver("DETCHAR	  =	0x%x.\n", eem_read(0x1100b208));
	idvfs_ver("AGECHAR	  =	0x%x.\n", eem_read(0x1100b20c));
	idvfs_ver("DCVALUES  =	0x%x.\n", eem_read(0x1100b240));
	idvfs_ver("AGEVALUES =	0x%x.\n", eem_read(0x1100b244));
	idvfs_ver("EEMEN	  =	0x%x.\n", eem_read(0x1100b238));
}
#endif
/************* for ptp1	temp *************/

/* static unsigned int func_lv_mask_idvfs	= (FUNC_LV_MODULE |	FUNC_LV_CPUFREQ
 | FUNC_LV_API | FUNC_LV_LOCAL | FUNC_LV_HELP); */
/* static unsigned int func_lv_mask_idvfs	= 0; */

static struct CHANNEL_STATUS channel_status[IDVFS_CHANNEL_NP] =	{
		[IDVFS_CHANNEL_SW] = {
			.name  = __stringify(IDVFS_CHANNEL_SW),
			.ch_number = IDVFS_CHANNEL_SW,
			.status	= 0,
		},

		[IDVFS_CHANNEL_OTP]	= {
			.name  = __stringify(IDVFS_CHANNEL_OTP),
			.ch_number = IDVFS_CHANNEL_OTP,
			.status	= 0,
		},

		[IDVFS_CHANNEL_OCP]	= {
			.name  = __stringify(IDVFS_CHANNEL_OCP),
			.ch_number = IDVFS_CHANNEL_OCP,
			.status	= 0,
		},
	};

static struct IDVFS_INIT_OPT idvfs_init_opt	= {
	.idvfs_en =	0,
	.freq_max =	3000,
	.freq_min =	500,
	.freq_cur =	0,
	.swavg_length =	0,
	.swavg_endis = 0,
	.channel = channel_status,
	};

/*	iDVFSAPB function ****************************************************** */
static unsigned	int	iDVFSAPB_Read(unsigned int sw_pAddr)
{
	unsigned int i = 0;

	/* chekc iDVFS disable */
	idvfs_write(0x11017000,	((unsigned int)(sw_pAddr)));
	idvfs_write(0x1101700c,	((unsigned int)(0x01)));

	while (i++ < 10) {
		if (0x01 == (idvfs_read(0x11017020) & 0x1f))
			return idvfs_read(0x11017010);
		udelay(5);
		idvfs_ver("wait 5usec count = %d.\n", i);
	}

	idvfs_error("[iDVFSAPB_Read] Addr	0x%x, wait timeout 50usec.\n", sw_pAddr);
	return -1;
}

/* return 0: Ok, -1: timeout */
static int iDVFSAPB_Write(unsigned int sw_pAddr, unsigned int sw_pWdata)
{
	unsigned int i = 0;

	idvfs_write(0x11017000,	((unsigned int)(sw_pAddr)));  /* addr, soft	reset */
	idvfs_write(0x11017004,	((unsigned int)(sw_pWdata))); /* data */
	idvfs_write(0x11017008,	((unsigned int)(0x01)));

	while (i++ < 10) {
		if (0x01 == (idvfs_read(0x11017020) & 0x1f))
			return 0;
		udelay(5);
		idvfs_ver("wait 5usec count = %d.\n", i);
	}

	idvfs_error("[iDVFSAPB_Write]	Addr 0x%x =	0x%x, and timeout 50us.\n",	sw_pAddr, sw_pWdata);
	return -1;
}

/* it's	only for DA9214	PMIC */
static int iDVFSAPB_DA9214_write(unsigned int sw_pAddr,	unsigned int sw_pWdata)
{
	unsigned int i = 0;

	/* iDVFSAPB_Write(0x90,	0x20);		*/
	/* iDVFSAPB_Write(0x94,	0x0102);	*/
	/* iDVFSAPB_Write(0x98,	0x0001);	*/

	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84	*/
	iDVFSAPB_Write(0x80, (sw_pAddr & 0xff));
	iDVFSAPB_Write(0x80, (sw_pWdata	& 0xff));
	iDVFSAPB_Write(0xa4, 0x0001);

	while (i++ < 10)	{
		if (0x00 == iDVFSAPB_Read(0xa4)) {
			/* clear IRQ status	*/
			iDVFSAPB_Write(0x8c, 0x0001);
			return 0;
		}
		udelay(5);
		idvfs_ver("wait 5usec count = %d.\n", i);
	}

	idvfs_error("[iDVFSAPB_DA9214_writ] Addr 0x%x	= 0x%x,	wait timeout 50usec.\n", sw_pAddr, sw_pWdata);
	return -1;
}

#if	0
/* it's only	for	DA9214 PMIC	but	not	work when iDVFS	enable!!! */
static int iDVFSAPB_DA9214_read(unsigned int sw_pAddr)
{
	unsigned int i = 0;

	/* dir change */
	iDVFSAPB_Write(0x90, 0x0010);
	/* transfer_aux_len	and	transfer_len */
	iDVFSAPB_Write(0x94, 0x0101);
	/* 2 transac: 1byte	TX then	1 byte RX */
	iDVFSAPB_Write(0x98, 0x0002);
	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84	*/
	iDVFSAPB_Write(0x80, (sw_pAddr & 0xff));
	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84	*/
	iDVFSAPB_Write(0xa4, 0x0001);

	while (i++ < 10) {
		if (0x00 == iDVFSAPB_Read(0xa4))
			return iDVFSAPB_Read(0x80);
		udelay(5);
		idvfs_ver("wait 5usec count = %d.\n", i);
	}

	idvfs_error("[iDVFSAPB_DA9214_read] Addr 0x%x, wait timeout 50usec.\n", sw_pAddr);
	return -1;
}

static int iDVFSAPB_pmic_manual_big(int	volt_mv) /*	it's only for DA9214 PMIC  */
{
	int	i;

	/* voltage range 300 ~ 1200mv */
	if ((volt_mv > 1200) | (volt_mv < 300))
		return -1;

	iDVFSAPB_DA9214_write(0xd9,	(DA9214_MV_TO_STEP(volt_mv)	| 0x0080));
}
#endif

static int iDVFSAPB_init(void) /* it's only	for	DA9214 PMIC	*/
{
	/* set speed 400K	= 0x1303, 3.4M = 0x1001	*/
	iDVFSAPB_Write(0xa0, 0x1303);
	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84  */
	iDVFSAPB_Write(0x84, 0x00d0);
	idvfs_ver("iDVFS Timming ctrl(0xa0) = 0x%x.\n", iDVFSAPB_Read(0xa0));
	idvfs_ver("iDVFS Slave	address(0x84) =	0x%x.(DA9214(0xd0) or MT6313(0xd6))\n",	iDVFSAPB_Read(0x84));

	/*HW pmic volt ctrl reg addr, DA9214: L/LL = 0xd7, Big = 0xd9, MT6313: L/LL = 0x??, Big = 0x96? */
	idvfs_write(0x11017014,	((unsigned int)(0xd9)));

	/* ok */
	return 0;
}

/* iDVFSAPB	function *********************************************************************** */

/* Freq	meter ****************************************************************************** */
unsigned int _mt_get_cpu_freq_idvfs(unsigned int num)
{
	int	output = 0,	i =	0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	/* CLK_DBG_CFG=0x1000010C	*/
	clk_dbg_cfg	= idvfs_read(0x1000010c);
	/* sel abist_cksw and enable freq meter sel abist */
	idvfs_write(0x1000010c,	(clk_dbg_cfg & 0xFFFFFFFE)|(num	<< 16));
	/* sel abist_cksw	and	enable freq	meter sel abist	*/
	/* DRV_WriteReg32(0x1000010c, (clk_dbg_cfg & 0xFFFFFFFC)|(num << 16)); */

	/* CLK_MISC_CFG_0=0x10000104 */
	clk_misc_cfg_0 = idvfs_read(0x10000104);
	/* select divider,	WAIT CONFIRM */
	idvfs_write(0x10000104,	(clk_misc_cfg_0	& 0x01FFFFFF));

	/* CLK26CALI_1=0x10000224 */
	clk26cali_1	= idvfs_read(0x10000224);
	/* cycle count default 1024,[25:16]=3FF */
	/* DRV_WriteReg32(CLK26CALI_1, 0x00ff0000);	*/

	/* temp	= DRV_Reg32(CLK26CALI_0); */
	idvfs_write(0x10000220,	0x1000); /*	CLK26CALI_0=0x10000220 */
	idvfs_write(0x10000220,	0x1010);


	/* wait	frequency meter	finish */
	while (idvfs_read(0x10000220) &	0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = idvfs_read(0x10000224) &	0xFFFF;
	/* Khz */
	output = (((temp * 26000)) / 1024 *	2);

	idvfs_write(0x1000010c,	clk_dbg_cfg);
	idvfs_write(0x10000104,	clk_misc_cfg_0);
	idvfs_write(0x10000220,	0x1010);
	idvfs_write(0x10000220,	0x1000);
	/* idvfs_write(0x10000220, clk26cali_0); */
	/* idvfs_write(0x10000224, clk26cali_1); */

	/* cpufreq_dbg("CLK26CALI_1	= 0x%x,	CPU	freq = %d KHz\n", temp,	output); */

	if (i > 10) {
		/* cpufreq_dbg("meter not finished!\n"); */
		return 0;
	} else {
		return output;
	}
}
/*	Freq meter *********************************************************************** */

/* iDVFSAPB	function ***********************************************************************  */
/* iDVFS function code */
/* if return 0:ok, -1:invalid parameter, -2: iDVFS is enable */
int	BigiDVFSEnable(unsigned int Fmax, unsigned int cur_vproc_mv_x100, unsigned int cur_vsram_mv_x100)
{
	int	rc = 0;

	/* set VSRAM to	1000 and fix */
	/* set Vproc to	1000 and fix */
	/* manual ctrl freq	first */
	/* temp:: before need enable ptp1 function */
	/* enable and initial idvfsapb i2c ctrl	*/

	iDVFSAPB_init();

	/* check parameter move	to this	API	*/
	if ((Fmax <	510) ||	(Fmax >	3000)) {
		idvfs_error("Fmax	= %d out of	510	~ 3000MHz range.\n", Fmax);
		/* freq	out	of range */
		return -1;
	}

	/* if (idvfs_init_opt.idvfs_en == 1)	*/
	if ((idvfs_read(0x10222470) & 0x1) == 1)	{
		idvfs_error("iDVFS already enable.\n");
		/* already enable */
		return -2;
	}
	if ((cur_vproc_mv_x100 < 50000)	|| (cur_vproc_mv_x100 >	120000)) {
		idvfs_error("Vproc_mv	= %d(x100mv) out of	range(50000	~ 120000).\n", cur_vproc_mv_x100);
		/* vproc out of	range */
		return -3;
	}
	if ((cur_vsram_mv_x100 < 50000)	|| (cur_vsram_mv_x100 >	120000)) {
		idvfs_error("Vsram_mv	= %d(x100mv) out of	range(50000	~ 120000).\n", cur_vsram_mv_x100);
		/* vrram out of	range */
		return -4;
	}
	/* check pos div only 0	or 1 */
	if (((idvfs_read(0x102224a0)	& 0x00007000) >> 12) >=	2) {
		idvfs_error("iDVFS enable	pos	div	= %d, (only	0 or 1).\n",
				 ((idvfs_read(0x102224a0) &	0x00007000)	>> 12));
		/* pos div error */
		return -5;
	}

	/* call	smc	function_id	= SMC_IDVFS_BigiDVFSEnable(Fmax, Vproc_mv_x100,	Vsram_mv_x100) */
	rc = SEC_BIGIDVFSENABLE(Fmax, cur_vproc_mv_x100, cur_vsram_mv_x100);

	if (rc == 0)	{
		/* max freq	equ	input FMax */
		idvfs_init_opt.freq_max	= Fmax;
		/* default 100%	freq start and enable sw channel */
		idvfs_init_opt.freq_cur	= Fmax;

		/* enable struct idvfs_en =	1 */
		idvfs_init_opt.idvfs_en	= 1;
		/* enable sw channel status	by struct */
		idvfs_init_opt.channel[IDVFS_CHANNEL_SW].status	= 1;
		/* clear oct/otpl channel status by	struct */
		idvfs_init_opt.channel[IDVFS_CHANNEL_OTP].status = 0;
		idvfs_init_opt.channel[IDVFS_CHANNEL_OCP].status = 0;

		idvfs_ver("iDVFS enable success. Fmax = %d	MHz	(Range:	500	~ 3000MHz).\n",	Fmax);
	} else {
		idvfs_error("iDVFS enable	fail and return	= %d\n", rc);
	}

	return rc;
}

/* return 0: Ok, -1: timeout */
int	BigiDVFSDisable(void)
{
	int	rc = 0,	i;

	/* call	smc	*/ /* function_id =	SMC_IDVFS_BigiDVFSDisable */
	rc = SEC_BIGIDVFSDISABLE();

	if (rc == 0)	{
		/* disable struct */
		idvfs_init_opt.idvfs_en	= 0;

		/* clear all channel status	by struct */
		for (i = 0; i < IDVFS_CHANNEL_NP; i++)
			idvfs_init_opt.channel[i].status = 0;

		idvfs_ver("iDVFS disable success.\n");
	} else {
		idvfs_error("iDVFS disable fail and return = %d\n", rc);
	}
	return rc;
}

/* return 0:Ok,	-1:	Invalid	Parameter */
int	BigiDVFSChannel(unsigned int Channelm, unsigned int EnDis)
{
/* static int BigiDVFSChannel(enum idvfs_channel Channelm, int EnDis)
{
	if ((Channelm <	0) || (Channelm	> 2))
		return -1;

	if ((idvfs_read(0x10222470) & 0x1) == 0) {
		idvfs_error("iDVFS is	disable.\n");
		return -2;
	}
*/

	/* call	smc	*/
	/* function_id = SMC_IDVFS_BigiDVFSChannel */
	/* rc =	SEC_BIGIDVFSCHANNEL(Channelm, EnDis); */
	/* setting register	*/
	switch (Channelm) {
	case 0:
		idvfs_write_field(0x10222470, 1:1, EnDis);
		break;

	case 1:
		idvfs_write_field(0x10222470, 2:2, EnDis);
		break;

	case 2:
		idvfs_write_field(0x10222470, 3:3, EnDis);
		break;

	}

	/* setting struct */
	idvfs_init_opt.channel[Channelm].status	= EnDis;
	idvfs_ver("iDVFS channel %s select	success, EnDis = %d.\n", idvfs_init_opt.channel[Channelm].name,	EnDis);
	return 0;
}

/* return 0	Ok.	-1 Error. Invalid parameter. No	action taken. */
/* This	function enables SW	to set the frequency of	maximum	unthrottled	operating */
/* frequency percentage. The percentage	is relative	to the FMaxMHz set when	*/
/* enabled.	The	IDVFS SW channel must be enabled before	using this function. */
int	BigIDVFSFreq(unsigned int Freqpct_x100)
{
	unsigned int freq_swreq;

	/* check freqpct */
	/* spec is 2041~10000 percentag */
	if ((Freqpct_x100 < (100)) || (Freqpct_x100 > (10000))) {
		idvfs_error("iDVFS FreqPct_x100 =	%d%%, range	must be	1%%~100%%.\n", Freqpct_x100);
		return -1;
	}

	if ((idvfs_read(0x10222470) & 0x3) != 3) {
		idvfs_error("iDVFS is	disable	or SW channel not select.\n");
		return -2;
	}

	/* get max freq	and	set	fpct */
	freq_swreq = (Freqpct_x100 / 100) << 12;
	if ((Freqpct_x100 % 100)	< 25)
		freq_swreq |= 0x0;
	else if ((Freqpct_x100 %	100) < 50)
		freq_swreq |= 0x400;
	else if ((Freqpct_x100 %	100) < 75)
		freq_swreq |= 0x800;
	else
		freq_swreq |= 0xc00;	/* if ((Freqpct_x100 % 100) < 99) */

	/* call	smc	*/
	/* function_id = SMC_IDVFS_BigiDVFSFreq */
	/* rc =	SEC_BIGIDVFSFREQ(Freqpct_x100);	*/
	/* Frepct_x100 = 100(1%) ~ 10000(100%) */
	/*	swreq =	cur/max	*/
	idvfs_write(0x10222498,	freq_swreq);

	idvfs_init_opt.freq_cur	= (idvfs_init_opt.freq_max * Freqpct_x100) / 10000;
	idvfs_ver("iDVFS freq cur setting success.	Freq_cur = %dMHz.\n", idvfs_init_opt.freq_cur);
	return 0;
}

/* return 0:. Ok, -1: Parameter	invalid	*/
int	BigiDVFSSWAvg(unsigned int Length, unsigned int EnDis)
{
	if ((Length < 0)	|| (Length > 7)) {
		idvfs_error("iDVFS SWAvg length must be 0~7 or Endis invalid.\n");
		return -1;
	}

	if ((EnDis <	0) || (EnDis > 1)) {
		idvfs_error("Enab	out	of range..\n");
		return -2;
	}

	/* call	smc	*/ /* function_id =	SMC_IDVFS_BigiDVFSSWAvg	*/
	/* rc =	SEC_BIGIDVFSSWAVG(Length, EnDis); */
	idvfs_write(0x102224cc,	((Length <<	4) | (EnDis)));

	idvfs_init_opt.swavg_length	= Length;
	idvfs_init_opt.swavg_endis = EnDis;
	idvfs_ver("iDVFS setting SWAvg	success.\n");
	return 0;
}

/* return value	of freq	percentage x 100, 0(0%)	~ 10000(100%), -1: SWAvg not enabled */
int	BigiDVFSSWAvgStatus(void)
{
	unsigned int freqpct_x100, sw_avgfreq =	0;

	/* if (rg_sw_avgfreq_en_r() == 0) { */
	/*	   return -1; */ /*	SWAvg not enable */
	/* } */

	/* call	smc, function_id = SMC_IDVFS_BigiDVFSSWAvgStatus */
	sw_avgfreq = ((idvfs_read(0x102224cc) >> 16) & 0x7fff);
	freqpct_x100 = (sw_avgfreq >> 8) * 100;
	freqpct_x100 +=	((sw_avgfreq & 0x80) ==	0x80) ?	50 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x40) ==	0x40) ?	25 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x20) ==	0x20) ?	13 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x10) ==	0x10) ?	6 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x8)	== 0x8)	? 3 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x4)	== 0x4)	? 2 : 0;
	freqpct_x100 +=	((sw_avgfreq & 0x2)	== 0x2)	? 1 : 0;

	if (freqpct_x100 == 0) {
		idvfs_init_opt.freq_cur	= 0;
		idvfs_ver("iDVFS or SW	AVG	not	enable,	SWAVG =	0.\n");
	} else if (freqpct_x100 > 0) {
		idvfs_init_opt.freq_cur	= ((idvfs_init_opt.freq_max * freqpct_x100) / 10000);
		idvfs_ver("iDVFS setting SWAvg	success	and	Freq_pet_x100 =	%d(%%),	Freq_cur = %dMHz.\n",
					freqpct_x100, idvfs_init_opt.freq_cur);
	}
	return freqpct_x100;
}

/* return 0: Ok, -1: freq out of range,	option function	*/
int	BigiDVFSPureSWMode(unsigned int function, unsigned int parameter)
{	int	rc = 0;

	/* func_para[0]: 0 for Freq, func_para[1] =	500	~ 3000Mhz,
				     disable iDVFS and manual	ctrl freq/volt	*/
	/* func_para[0]: 1 for Volt, func_para[1] =	600	~ 1150mv(sarm add 100mv) */

	/* temp	to modify */
	/* rc =	SEC_BIGIDVFSPURESWMODE(function, parameter); */


	if (rc == 0)
		idvfs_ver("iDVFS return SWMode	success.\n");
	else
		idvfs_error("iDVFS return	SWMode fail	and	return = %d\n",	rc);

	return rc;
}

int	BigiDVFSPllSetFreq(unsigned int Freq)
{
	int	rc = 0;

	if ((Freq <	250) ||	(Freq >	3000)) {
		idvfs_error("Output Freq = %d	out	of 250 ~ 3000MHz range.\n",	Freq);
		return -1;
	}

	if ((idvfs_read(0x10222470) & 0x1) == 1)	{
		idvfs_error("PLL cannot set when iDVFS is	enable.\n");
		return -2;
	}

	rc = SEC_BIGIDVFSPLLSETFREQ(Freq);

	if (rc >= 0)
		idvfs_ver("iDVFS setting PLL Freq success.	Freq = %dMHz.\n", Freq);
	else
		idvfs_error("iDVFS setting PLL Freq error	and	return = %d\n",	rc);

	return rc;
}

unsigned int BigiDVFSPllGetFreq(void)
{
	unsigned int freq =	0, pos_div = 0;

	/* default fcur =	1500MHz	*/
	freq = (((unsigned long	long)(idvfs_read(0x102224a4) & 0x7fffffff) * 26L) / (1L <<	24));
	pos_div	= (1 << ((idvfs_read(0x102224a0) >> 12) & 0x7));

	idvfs_ver("iDVFS get PLL Freq = %dMHz,	pos_div	= %d, output Freq =	%dMHZ.\n",
				freq, pos_div, (freq / pos_div));

	return (freq / pos_div);
}

int	BigiDVFSPllDisable(void)
{

	if ((idvfs_read(0x102224a0) & 0x1) == 0)	{
		idvfs_error("PLL already disable.\n");
		return -1;
	}

	idvfs_write_field(0x102224a0, 0:0, 0);
	udelay(1);
	idvfs_write_field(0x102224a0, 8:8, 0);
	udelay(1);

	idvfs_ver("iDVFS PLL disable success.\n");
	return 0;
}

unsigned int BigiDVFSSRAMLDOEFUSE(void)
{
	return (idvfs_read(0x1020666C) & 0xffff);
}

int	BigiDVFSSRAMLDOSet(unsigned int mVolts_x100)
{
	int	rc = 0;

	if ((mVolts_x100	< 50000) ||	(mVolts_x100 > 120000))	{
		idvfs_error("Error: SRAM LDO volte = %dmv, out of	range 500mv~1200mv.\n",	mVolts_x100);
		return -1; /* out of range	*/
	}

	rc = SEC_BIGIDVFSSRAMLDOSET(mVolts_x100);

	if (rc >= 0)
		idvfs_ver("SRAM LDO setting = %dmv_x100 success.\n", mVolts_x100);
	else
		idvfs_error("iDVFS set SRAM LDO volt fail	and	return rc =	%d.\n",	rc);

	return rc;
}

unsigned int BigiDVFSSRAMLDOGet(void)
{
	int	vosel =	0, volt;

	vosel =	(idvfs_read(0x102222b0)	& 0xf);

	switch (vosel) {
	case 0:
		volt = 105000;
		break;

	case 1:
		volt = 60000;
		break;

	case 2:
		volt = 70000;
		break;

	default:
		volt = (((vosel	- 3) * 2500) + 90000);
		break;
	}

	idvfs_ver("SRAM LDO get = %d(mv_x100).\n",	volt);

	return volt;
}

int	BigiDVFSPOSDIVSet(unsigned int pos_div)
{
	if ((pos_div > 7) ||	(pos_div < 0)) {
		idvfs_error("iDVFS Pos Div set = %d, out of range	0 ~	7.\n", pos_div);
		return -1;
	}

	idvfs_write_field(0x102224a0, 0:0, 0);			/* set ARMPLL disable */
	idvfs_write_field(0x102224a0, 14:12, pos_div);	/* set ARMPLL_CON0 bit[12:14] for pos_div */
	idvfs_write_field(0x102224a0, 0:0, 1);			/* set ARMPLL enable */

	return 0;
}

unsigned int BigiDVFSPOSDIVGet(void)
{
	return ((idvfs_read(0x102224a0)	>> 12) & 0x7);	  /* 0~7 */
}

int	BigiDVFSPLLSetPCM(unsigned int freq)	/* <1000 ~ = 3000(MHz),	with our pos div value */
{

	if ((freq > 3000) ||	(freq <= 1000))	{
		idvfs_error("Source PLL Freq = %d	out	of 1000	~ 3000MHz range.\n", freq);
		return -1;
	}

	idvfs_write_field(0x102224a0, 0:0, 0); /* set ARMPLL disable */
	udelay(1);
	idvfs_write_field(0x102224a0, 0:0, 1); /* set ARMPLL enable	*/
	udelay(1);

	/* pllsdm= cur freq	= max freq */
	idvfs_write_field(0x102224a4, 30:0,
				(((unsigned	long long)(1L << 24) * (unsigned long long)freq) /	26));
	udelay(1);

	idvfs_write_field(0x102224a4, 31:31, 1); /* toggle 1 */
	udelay(1);
	idvfs_write_field(0x102224a4, 31:31, 0); /* toggle 0 */
	udelay(1);

	/* idvfs_ver("Bigpll setting ARMPLL_CON0 =	0x%x,
	ARMPLL_CON1_PTP3 = 0x%x, Freq	= %dMHz, POS_DIV = %d.\n",
	ptp3_reg_read(ARMPLL_CON0_PTP3), ptp3_reg_read(ARMPLL_CON1_PTP3),
	freq, rg_armpll_posdiv_r()); */

	return 0;
}

unsigned int BigiDVFSPLLGetPCW(void) /*	 <1000 ~ = 3000(MHz), with our pos div value */
{
	unsigned int freq;

	/* default fcur	= 1500MHz */
	freq = (((unsigned long	long)(idvfs_read(0x102224a4) & 0x7fffffff) * 26L) /	(1L	<< 24));

	idvfs_ver("Get	PLL	PCW	without	pos_div	and	clk_div	freq = %d.\n", freq);
	return freq;
}

/* for iDVFS interrupt ISR handler */
int BigiDVFSISRHandler(void)
{
	if ((idvfs_read(0x10222470) & 0x00004000) != 0) {
		idvfs_ver("iDVFS Vporc timeout interrupt. iDVFS	ctrl = 0x%x, clear irq now....\n",
				   idvfs_read(0x10222470));
		WARN_ON(1);

		/* clear interrupt status */
		idvfs_write_field(0x10222470, 11:11, 1);
		idvfs_write_field(0x10222470, 11:11, 0);

		/* force clear and start */
		idvfs_write_field(0x10222470, 11:11, 1);
		udelay(1);
		idvfs_write_field(0x10222470, 11:11, 1);
		udelay(1);

		/* wait	interrupt status cleawr	*/
		/* while();	*/
	}

	return 0;
}

/* iDVFS ISR Handler */
static irqreturn_t idvfs_big_isr(int irq, void *dev_id)
{
	/* FUNC_ENTER(FUNC_LV_MODULE); */

	/* print status	*/
	idvfs_ver("interrupt number: 331 for idvfs or otp interrupt trigger.\n");
	BigOTPISRHandler();
	BigiDVFSISRHandler();

	/* FUNC_EXIT(FUNC_LV_MODULE); */
	return IRQ_HANDLED;
}

/* enable debug	message	*/
#define	DEBUG	0

/* Device infrastructure */
static int idvfs_remove(struct platform_device *pdev)
{
	return 0;
}

static int idvfs_probe(struct platform_device *pdev)
{
	int	err	= 0;

	idvfs_ver("IDVFS Probe	Initial.\n");
	idvfs_irq_number1 =	0;

	/* set iDVFS IRQ */
	err	= request_irq(idvfs_irq_number1, idvfs_big_isr,	IRQF_TRIGGER_HIGH, "idvfs_big_isp",	NULL);
	if (err) {
		idvfs_error("iDVFS IRQ register failed: idvfs_isr_big	(%d)\n", err);
		WARN_ON(1);
	}

	return 0;
}


static int idvfs_suspend(struct	platform_device	*pdev, pm_message_t	state)
{
	/*
	kthread_stop(idvfs_thread);
	*/

	idvfs_ver("IDVFS suspend\n");
	return 0;
}


static int idvfs_resume(struct platform_device *pdev)
{
	/*
	idvfs_thread = kthread_run(idvfs_thread_handler, 0,	"idvfs xxx");
	if (IS_ERR(idvfs_thread))
	{
		printk("[%s]: failed to	create idvfs xxx thread\n",	__func__);
	}
	*/

	idvfs_ver("IDVFS resume\n");
	return 0;
}

struct platform_device idvfs_pdev =	{
	.name	= "mt_idvfs",
	.id		= -1,
};

static struct platform_driver idvfs_pdrv = {
	.remove		= idvfs_remove,
	.shutdown	= NULL,
	.probe		= idvfs_probe,
	.suspend	= idvfs_suspend,
	.resume		= idvfs_resume,
	.driver		= {
		.name	= "mt_idvfs",
	},
};


/* ----------------------------------------------------------------------------- */

#ifdef CONFIG_PROC_FS

static char	*_copy_from_user_for_proc(const	char __user	*buffer, size_t	count)
{
	char *buf =	(char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;
	if (count >= PAGE_SIZE)
		goto out;
	if (copy_from_user(buf,	buffer,	count))
		goto out;

	buf[count] = '\0';
	return buf;

out:
	free_page((unsigned	long)buf);
	return NULL;
}

/* idvfs_debug */
static int idvfs_debug_proc_show(struct	seq_file *m, void *v)
{
	seq_printf(m, "IDVFS	debug (log level) =	%d\n", func_lv_mask_idvfs);
	return 0;
}

static ssize_t idvfs_debug_proc_write(struct file *file, const char	__user *buffer,	size_t count, loff_t *pos)
{
	char *buf =	_copy_from_user_for_proc(buffer, count);
	unsigned int dbg_lv;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &dbg_lv))
		func_lv_mask_idvfs = dbg_lv;
	else
		idvfs_error("echo	dbg_lv (dec) > /proc/idvfs/idvfs_debug\n");

	free_page((unsigned	long)buf);
	return count;
}

/* dvt_test	*/
static int dvt_test_proc_show(struct seq_file *m, void *v)
{
	unsigned char ret_val = 0;
	/* unsigned int armplldiv_mon_en; */
	/* seq_printf(m, "empty.\n"); */

	/* get freq	cur	*/
	/* BigiDVFSSWAvgStatus(); */

	/* print struct	IDVFS_INIT_OPT */

#if	0
	seq_printf(m, "IDVFS_INIT_OPT\n"
			"idvfs_en =	%d\n"
			"freq_max =	%d MHz\n"
			"freq_min =	%d MHz\n"
			"freq_cur =	%d MHz\n"
			"swavg_length =	%d,	enable = %d\n",
			idvfs_init_opt.idvfs_en,
			idvfs_init_opt.freq_max,
			idvfs_init_opt.freq_min,
			idvfs_init_opt.freq_cur,
			idvfs_init_opt.swavg_length,
			idvfs_init_opt.swavg_endis);

	/* print channel status	and	name */
	for (i = 0; i < IDVFS_CHANNEL_NP; i++)
		seq_printf(m, "Channel name	=  %s, CH_Number = %d, EnDIS = %d.\n",
		idvfs_init_opt.channel[i].name,
		idvfs_init_opt.channel[i].ch_number,
		idvfs_init_opt.channel[i].status);
#endif

	seq_puts(m, "======================================================\n");
	seq_printf(m, "iDVFS ctrl =	0x%x.\n", idvfs_read(0x10222470));
	seq_printf(m, "iDVFS debugout =	0x%x.\n", idvfs_read(0x102224c8));
	seq_printf(m, "SW AVG status(%%_x100) =	%d.\n",	BigiDVFSSWAvgStatus());

	/* enable all freq meter and get freq */
	/* armplldiv_mon_en = idvfs_read(0x1001a284); */
	/* add for enable all monitor channel */
	/* idvfs_write(0x1001a284,	0xffffffff); */
	seq_printf(m, "Big Freq	meter =	%dKHZ.\n", _mt_get_cpu_freq_idvfs(37));	/* or 46 */
	seq_printf(m, "LL Freq meter  =	%dKHZ.\n", _mt_get_cpu_freq_idvfs(34));	/* 42 */
	seq_printf(m, "L Freq meter	  =	%dKHZ.\n", _mt_get_cpu_freq_idvfs(35));	/* 43 */
	seq_printf(m, "CCI Freq	meter =	%dKHZ.\n", _mt_get_cpu_freq_idvfs(36));	/* 44 */
	seq_printf(m, "Back	Freq meter=	%dKHZ.\n", _mt_get_cpu_freq_idvfs(45));
	/* idvfs_write(0x1001a284,	armplldiv_mon_en); */

	da9214_read_interface(0xd9, &ret_val, 0x7f, 0);
	seq_printf(m, "Big Vproc = %dmv.\n", DA9214_STEP_TO_MV((ret_val & 0x7f)));
	seq_printf(m, "Big Vsram = %dmv.\n", (BigiDVFSSRAMLDOGet()/100));
	/* switch bank 0; */
	eem_write(0x1100b400, 0x003f0000);
	seq_printf(m, "DCVALUES	 =	0x%x.\n", eem_read(0x1100b240));
	seq_printf(m, "EEMEN	  =	0x%x.\n", eem_read(0x1100b238));

	/* reg dump	*/
	/* for(i = 0; i	< 24; i++) */
	/*		seq_printf(m, "Reg 0x%x	= 0x%x.\n",	(i + 0x470), idvfs_read(0x10222470 + i)); */
	seq_puts(m, "======================================================\n");

	return 0;
}

static ssize_t dvt_test_proc_write(struct file *file, const	char __user	*buffer, size_t	count, loff_t *pos)
{
	char *buf =	(char *)_copy_from_user_for_proc(buffer, count);
	int	rc = 0xff, err = 0;
	unsigned int func_code,	func_para[3];

	if (!buf)
		return -EINVAL;

	/* get scanf length and parameter */
	err = sscanf(buf, "%d %d %d %d", &func_code, &func_para[0], &func_para[1], &func_para[2]);
	if (err > 0) {
		/* print input information */
		idvfs_info("Input = %d, Para = %d, %d, %d.\n", err, func_para[0], func_para[1], func_para[2]);

		/* switch function code */
		switch (func_code) {
		case 0:
			/* FMax	= 500 ~	3000, Vproc_mv_x100	= 50000~120000,	VSram_mv_x100 =	50000~120000 */
			if (err == 4)
				rc = BigiDVFSEnable(func_para[0], func_para[1],	func_para[2]);
			break;
		case 1:
			/* Disable iDVFS */
			if (err == 1)
				rc = BigiDVFSDisable();
			break;
		case 2:
			/* Manual ctrl freq */
			if (err == 2)
				rc = BigIDVFSFreq(func_para[0]);
			break;
		case 3:
			/* Channel = 0(SW),	1(OCP),	2(OTP),	EnDis =	0/1	*/
			if (err == 3)
				rc = BigiDVFSChannel(func_para[0], func_para[1]);
			break;
		case 4:
			/* Length =	0~7, EnDis = 0/1 */
			if (err == 3)
				rc = BigiDVFSSWAvg(func_para[0], func_para[1]);
			break;
		/* case 5 : */
		/* case 6 : */
		case 7:
			/* Set L/LL Vproc, Vsarm mv_x100*/
			if (err == 3) {
				rc = da9214_vosel_buck_a(func_para[0]);
				/* set_cur_volt_sram_l(func_para[1]) */
			}
			break;
		case 8:
			/* Set Big Vproc, Vsram by mv_x100 */
			if (err == 3) {
				/*rc = da9214_vosel_buck_b(func_para[0]); */
				iDVFSAPB_init();
				iDVFSAPB_DA9214_write(0xd9, DA9214_MV_TO_STEP((func_para[0] / 100)));
				BigiDVFSSRAMLDOSet(func_para[1]);
			}
			rc = 0;
			break;
		case 9:
			/* reserve for ptp1, don't remove case 9, when ptp1	enable then	unrun this command */
			if (err == 1)
				eem_init_det_tmp();
			rc = 0;
			break;
		case 10:
			/* case 10: org idvfsapb, chg to Freq setting */
			if (err == 2)
				rc = BigiDVFSPllSetFreq(func_para[0]);
			break;
		case 16:
			/* DFD download, default 0x00011110 */
			if (err == 1)
				idvfs_write(0x10005500,	0x00033330);
			rc = 0;
			break;
		case 17:
			/* UDI to PMUX Mode 3(JTAG2), default 0x00011110 */
			if (err == 1) {
				idvfs_write(0x10005330, 0x331111);
				idvfs_write(0x10005340, 0x1100333);
			}
			rc = 0;
			break;
		case 18:
			/* UDI to PMUX Mode 4(SDCARD), default 0x00011110 */
			if (err == 1)
				idvfs_write(0x10005400, 0x04414440);
			rc = 0;
			break;
		case 19:
			/* AUXPIMX swithc */
			if (err == 3) {
				if (func_para[0] == 0) {
					if (func_para[1] == 0) {
						/* disable pinmux */
						idvfs_write_field(0x10002510, 30:30, 0x1);
						/* L/LL CPU AGPIO prob out volt disable */
						idvfs_write(0x10001fac, 0x000000ff);
					} else if (func_para[1] == 1) {
						/* enable pinmux */
						/* L/LL CPU AGPIO enable */
						idvfs_write_field(0x10002510, 30:30, 0x0);
						/* L/LL CPU AGPIO prob out volt enable */
						idvfs_write(0x10001fac, 0x0000ff00);
					}
				} else if (func_para[0] == 1) {
					if (func_para[1] == 0) {
						/* disable pinmux */
						/* L/LL CPU AGPIO prob out volt disable */
						idvfs_write_field(0x10002890, 6:6, 0x1);
						idvfs_write_field(0x102222b0, 31:31, 0x0);
					} else if (func_para[1] == 1) {
						/* enable pinmux */
						/* L/LL CPU AGPIO enable */
						idvfs_write_field(0x10002890, 6:6, 0x0);
						idvfs_write_field(0x102222b0, 31:31, 0x1);
					}
				}
			}
			break;
		}

		if (rc == 0xff)
			idvfs_error("Error input parameter or	function code.\n");
		else if	(rc	>= 0)
			idvfs_ver("Function code = %d, return success. return value = %d.\n", func_code, rc);
		else
			idvfs_ver("Function code = %d, fail and return value = %d.\n", func_code, rc);
	}

	free_page((unsigned	long)buf);
	return count;
}

#define	PROC_FOPS_RW(name)							\
	static int name	## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
	}									\
	static const struct	file_operations	name ##	_proc_fops = {		\
		.owner			= THIS_MODULE,					\
		.open			= name ## _proc_open,				\
		.read			= seq_read,					\
		.llseek			= seq_lseek,					\
		.release		= single_release,				\
		.write			= name ## _proc_write,				\
	}

#define	PROC_FOPS_RO(name)							\
	static int name	## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
	}									\
	static const struct	file_operations	name ##	_proc_fops = {		\
		.owner			= THIS_MODULE,					\
		.open			= name ## _proc_open,				\
		.read			= seq_read,					\
		.llseek			= seq_lseek,					\
		.release		= single_release,				\
	}

#define	PROC_ENTRY(name)	{__stringify(name),	&name ## _proc_fops}

PROC_FOPS_RW(idvfs_debug);
PROC_FOPS_RW(dvt_test);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int	i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry	entries[] =	{
		PROC_ENTRY(idvfs_debug),
		PROC_ENTRY(dvt_test),
	};

	dir	= proc_mkdir("idvfs", NULL);

	if (!dir) {
		idvfs_error("fail	to create /proc/idvfs @	%s()\n", __func__);
		return -ENOMEM;
	}

	for	(i = 0;	i <	ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO |	S_IWUSR	| S_IWGRP, dir,	entries[i].fops))
			idvfs_error("%s(), create	/proc/idvfs/%s failed\n", __func__,	entries[i].name);
	}

	return 0;
}
#endif /* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init idvfs_init(void)
{
	int	err	= 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,idvfs");
	if (node) {
		/* Setup IO	addresses */
		idvfs_base = of_iomap(node, 0);
		/*get idvfs	irq	num*/
		idvfs_irq_number1 =	irq_of_parse_and_map(node, 0);

		idvfs_ver("idvfs base = 0x%lx, irq = %d.\n", (unsigned long)idvfs_base, idvfs_irq_number1);
	}

	/* log level initial */
	func_lv_mask_idvfs = 0;

	/* register	platform device/driver */
	err	= platform_device_register(&idvfs_pdev);

	if (err) {
		idvfs_error("fail	to register	IDVFS device @ %s()\n",	__func__);
		goto out;
	}

	err	= platform_driver_register(&idvfs_pdrv);
	if (err) {
		idvfs_error("%s(), IDVFS driver callback register	failed..\n", __func__);
		return err;
	}

	/* ver = mt_get_chip_sw_ver(); */
	/* ptp2_lo_enable =	1; */
	/* turn_on_LO();  */

#ifdef CONFIG_PROC_FS
	/* init	proc */
	if (_create_procfs()) {
		err	= -ENOMEM;
		goto out;
	}
#endif /* CONFIG_PROC_FS */

out:
	return err;
}

static void	__exit idvfs_exit(void)
{
	idvfs_ver("IDVFS de-initialization\n");
	platform_driver_unregister(&idvfs_pdrv);
	platform_device_unregister(&idvfs_pdev);
}

module_init(idvfs_init);
module_exit(idvfs_exit);

MODULE_DESCRIPTION("MediaTek iDVFS Driver v0.1");
MODULE_LICENSE("GPL");

#undef __MT_IDVFS_C__
