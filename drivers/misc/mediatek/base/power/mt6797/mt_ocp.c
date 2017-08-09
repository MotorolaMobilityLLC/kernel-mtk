#define __MT_OCP_C__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/ktime.h>
#include <linux/io.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include "mach/irqs.h"
#include "mt_ocp.h"
#include "mt_idvfs.h"
#include "mt_cpufreq.h"
#include "mach/mt_thermal.h"
#include <mt-plat/sync_write.h>	/* mt_reg_sync_writel */
#include <mt-plat/mt_io.h>		/*reg read, write */
#include <mt-plat/aee.h>		/* ram console */
#include <linux/spinlock.h>

#include "../../../power/mt6797/da9214.h"
#define	DA9214_STEP_TO_MV(step)				(((step	& 0x7f)	* 10) +	300)

/*
 * BIT Operation
 */
#undef  BIT_OCP
#define BIT_OCP(_bit_)                    (unsigned)(1 << (_bit_))
#define BITS_OCP(_bits_, _val_)           ((((unsigned) -1 >> (31 - ((1) ? _bits_))) \
& ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define BITMASK_OCP(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define GET_BITS_VAL_OCP(_bits_, _val_)   (((_val_) & (BITMASK_OCP(_bits_))) >> ((0) ? _bits_))

/**
 * Read/Write a field of a register.
 * @addr:       Address of the register
 * @range:      The field bit range in the form of MSB:LSB
 * @val:        The value to be written to the field
 */

#define MTK_SIP_KERNEL_OCP_READ 0x8200035F
#define MTK_SIP_KERNEL_OCP_WRITE 0x8200035E
/* for ATF */
#define ocp_read(addr)                       mt_secure_call_ocp(MTK_SIP_KERNEL_OCP_READ, addr, 0, 0)
#define ocp_read_field(addr, range)          GET_BITS_VAL_OCP(range, ocp_read(addr))
#define ocp_write(addr, val)                 mt_secure_call_ocp(MTK_SIP_KERNEL_OCP_WRITE, addr, val, 0)
#define ocp_write_field(addr, range, val)    ocp_write(addr, \
(ocp_read(addr) & ~(BITMASK_OCP(range))) | BITS_OCP(range, val))
/* for IO MAP */
#define _ocp_read(addr)                      __raw_readl(IOMEM(addr))
#define _ocp_read_field(addr, range)         GET_BITS_VAL_OCP(range, _ocp_read(addr))
#define _ocp_write(addr, val)                 mt_reg_sync_writel((val), ((void *)addr))
#define _ocp_write_field(addr, range, val)    _ocp_write(addr, \
(_ocp_read(addr) & ~(BITMASK_OCP(range))) | BITS_OCP(range, val))


#define TAG     "[mt_ocp]"

#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define ocp_err(fmt, args...)    pr_err(ANDROID_LOG_ERROR, TAG, fmt, ##args)
	#define ocp_warn(fmt, args...)   pr_warn(ANDROID_LOG_WARN, TAG, fmt, ##args)
	#define ocp_info(fmt, args...)   pr_info(ANDROID_LOG_INFO, TAG, fmt, ##args)
	#define ocp_deb(fmt, args...)    pr_debug(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#else
	#define ocp_err(fmt, args...)     pr_err(TAG fmt, ##args)
	#define ocp_warn(fmt, args...)    pr_warn(TAG fmt, ##args)
	#define ocp_info(fmt, args...)    pr_info(TAG fmt, ##args)
	#define ocp_deb(fmt, args...)     pr_debug(TAG fmt, ##args)
#endif

/* adb command list
echo 6 0 1 > /proc/ocp/dvt_test //enable AUXPUMX for L/LL
echo 6 0 0 > /proc/ocp/dvt_test //disable AUXPUMX for L/LL
echo 6 1 1 > /proc/ocp/dvt_test //enable AUXPUMX for Big
echo 6 1 0 > /proc/ocp/dvt_test //disable AUXPUMX for Big
echo 6 2 1 > /proc/ocp/dvt_test //enable AUXPUMX for GPU
echo 6 2 0 > /proc/ocp/dvt_test //disable AUXPUMX for GPU

echo 7 > 1000 1100 /proc/ocp/dvt_test // L,LL Vproc, Vsram
echo 8 > 1000 1100 /proc/ocp/dvt_test // Big Vproc, Vsram

*/

#ifdef CONFIG_MTK_RAM_CONSOLE
	#define CONFIG_OCP_AEE_RR_REC 1
#endif




static unsigned int OCP_DEBUG_FLAG;
static unsigned int BIG_OCP_ON;
static unsigned int LITTLE_OCP_ON;
static unsigned int HW_API_DEBUG_ON;
static unsigned int IRQ_Debug_on;
static unsigned int Reg_Debug_on;
static unsigned int big_dreq_enable;
static unsigned int dvt_test_on;
static unsigned int dvt_test_set;
static unsigned int dvt_test_msb;
static unsigned int dvt_test_lsb;
static unsigned int hqa_test;
static unsigned int little_dvfs_on;
static unsigned int do_ocp_stress_test;
static unsigned int do_ocp_test_on;

/* OCP ADDR */
#ifdef CONFIG_OF
static void __iomem  *ocp_base;            /* 0x10220000 0x4000, OCP */
static void __iomem  *ocppin_base;         /* 0x10005000 0x1000, UDI pinmux reg */
static void __iomem  *ocpprob_base;        /* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */
static void __iomem  *ocpefus_base;        /* 0x10206000 0x1000, OCP eFUSE register */
static int ocp2_irq0_number; /* 279+50=329 */
static int ocp2_irq1_number; /* 280+50=330 */
static int ocp0_irq0_number; /* 282+50=332  LL = MP0 */
static int ocp0_irq1_number; /* 283+50=333  */
static int ocp1_irq0_number; /* 284+50=334  L = MP1  */
static int ocp1_irq1_number; /* 285+50=335  */
#endif

#define OCPPIN_BASE				(ocppin_base)
#define OCPPROB_BASE			(ocpprob_base)
#define OCPEFUS_BASE			(ocpefus_base)

/* 0x10005000 0x1000, DFD,UDI pinmux reg */
#define OCPPIN_UDI_JTAG1		(OCPPIN_BASE+0x330)
#define OCPPIN_UDI_JTAG2		(OCPPIN_BASE+0x340)
#define OCPPIN_UDI_SDCARD		(OCPPIN_BASE+0x400)

#if defined(CONFIG_OCP_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_ocp_aee_init(void)
{
	aee_rr_rec_ocp_2_target_limit(127999);
	aee_rr_rec_ocp_2_enable(9);
}
#endif


typedef struct OCP_OPT {
	unsigned short ocp_cluster0_flag;
	unsigned short ocp_cluster1_flag;
	unsigned short ocp_cluster2_flag;
	unsigned short ocp_cluster0_enable;
	unsigned short ocp_cluster1_enable;
	unsigned short ocp_cluster2_enable;
	unsigned int ocp_cluster2_OCPAPBCFG24;
} OCP_OPT;

static struct OCP_OPT ocp_opt = {
	.ocp_cluster0_flag = 0,
	.ocp_cluster1_flag = 0,
	.ocp_cluster2_flag = 0,
	.ocp_cluster0_enable = 0,
	.ocp_cluster1_enable = 0,
	.ocp_cluster2_enable = 0,
	.ocp_cluster2_OCPAPBCFG24 = 0,
};

typedef struct DVFS_STAT {
	unsigned int Freq;
	unsigned int Volt;
	unsigned int Thermal;
} DVFS_STAT;


typedef struct OCP_STAT {
	unsigned int IntEnDis;
	unsigned int IRQ1;
	unsigned int IRQ0;
	unsigned int CapToLkg;
	unsigned int CapOCCGPct;
	unsigned int CaptureValid;
	unsigned int CapTotAct;
	unsigned int CapMAFAct;
	unsigned int CGAvgValid;
	unsigned long long CGAvg;
	unsigned long long AvgLkg;
	unsigned int TopRawLkg;
	unsigned int CPU0RawLkg;
	unsigned int CPU1RawLkg;
	unsigned int CPU2RawLkg;
	unsigned int CPU3RawLkg;
} OCP_STAT;

static struct OCP_STAT ocp_status[3] = {
[0] = {
	.IntEnDis = 0,
	.IRQ1 = 0,
	.IRQ0 = 0,
	.CapToLkg = 0,
	.CapOCCGPct = 0,
	.CaptureValid = 0,
	.CapTotAct = 0,
	.CapMAFAct = 0,
	.CGAvgValid = 0,
	.CGAvg = 0,
	.AvgLkg = 0,
	.TopRawLkg = 0,
	.CPU0RawLkg = 0,
	.CPU1RawLkg = 0,
	.CPU2RawLkg = 0,
	.CPU3RawLkg = 0,
},
[1] = {
	.IntEnDis = 0,
	.IRQ1 = 0,
	.IRQ0 = 0,
	.CapToLkg = 0,
	.CapOCCGPct = 0,
	.CaptureValid = 0,
	.CapTotAct = 0,
	.CapMAFAct = 0,
	.CGAvgValid = 0,
	.CGAvg = 0,
	.AvgLkg = 0,
	.TopRawLkg = 0,
	.CPU0RawLkg = 0,
	.CPU1RawLkg = 0,
	.CPU2RawLkg = 0,
	.CPU3RawLkg = 0,
},
[2] = {
	.IntEnDis = 0,
	.IRQ1 = 0,
	.IRQ0 = 0,
	.CapToLkg = 0,
	.CapOCCGPct = 0,
	.CaptureValid = 0,
	.CapTotAct = 0,
	.CapMAFAct = 0,
	.CGAvgValid = 0,
	.CGAvg = 0,
	.AvgLkg = 0,
	.TopRawLkg = 0,
	.CPU0RawLkg = 0,
	.CPU1RawLkg = 0,
	.CPU2RawLkg = 0,
	.CPU3RawLkg = 0,
},
};

/* for HQA buffer */
#define NR_HQA 10000
static struct OCP_STAT ocp_hqa[3][NR_HQA];
static struct DVFS_STAT dvfs_hqa[3][NR_HQA];

/* BIG CPU */
int BigOCPConfig(int VOffInmV, int VStepInuV)
{
int ret;
ret = -1;

if (ocp_opt.ocp_cluster2_enable == 1)
	return 0;

if (VOffInmV < 0 || VOffInmV > 1000) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VOffInmV must be 0 ~ 1000 mV");

return -1;
}
if (VStepInuV < 1000 || VStepInuV > 100000) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VStepInuV must be 1000 ~ 100000 uV");

return -1;
}

spin_lock(&reset_lock);
if (reset_flags == 0)
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCONFIG, VOffInmV, VStepInuV, 0);
spin_unlock(&reset_lock);

if (ret == 0)
	return 0;
else
	return -1;

}


int BigOCPSetTarget(int OCPMode, int Target)
{
int ret;
ret = -1;

if ((Target < 0) || (Target > 127000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("Target must be 0 ~ 127000 mW)\n");

return -1;
}

if (!((OCPMode == OCP_ALL) || (OCPMode == OCP_FPI) || (OCPMode == OCP_OCPI))) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Invalid OCP Mode!\n");

return -1;
}

if (HW_API_DEBUG_ON)
	ocp_info("Set Cluster 2 limit = %d mW\n", Target);

#if defined(CONFIG_OCP_AEE_RR_REC) && !defined(EARLY_PORTING)
	aee_rr_rec_ocp_2_target_limit(Target);
#endif

if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
	return -1;

spin_lock(&reset_lock);
if (reset_flags == 0)
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPSETTARGET, OCPMode, Target, 0);
spin_unlock(&reset_lock);

if (ret == 0)
	return 0;
else
	return -1;

}


int BigOCPEnable(int OCPMode, int Units, int ClkPctMin, int FreqPctMin)
{
int ret;
ret = -1;

if (ocp_opt.ocp_cluster2_enable == 1)
	return 0;

if ((ClkPctMin < 625) || (ClkPctMin > 10000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("ClkPctMin must be 625 (6.25%%)~ 10000 (100.00%%)\n");

return -1;
}

if ((FreqPctMin < 0) || (FreqPctMin > 10000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("FreqPctMin must be 0 (0%%)~ 10000 (100.00%%)\n");

return -1;
}

if (!((OCPMode == OCP_ALL) || (OCPMode == OCP_FPI) || (OCPMode == OCP_OCPI))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("Invalid OCP Mode!\n");
		return -1;
}

if (Units == OCP_MA) {
	spin_lock(&reset_lock);
	if (reset_flags == 0)
		ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPENABLE0, OCPMode, ClkPctMin, FreqPctMin);
	spin_unlock(&reset_lock);
	if (ret == 0) {
			ocp_opt.ocp_cluster2_enable = 1;
			if (HW_API_DEBUG_ON)
				ocp_info("Cluster 2 enable.\n");
	return 0;
	}
} else if (Units == OCP_MW) {
	spin_lock(&reset_lock);
	if (reset_flags == 0)
		ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPENABLE1, OCPMode, ClkPctMin, FreqPctMin);
	spin_unlock(&reset_lock);
	if (ret == 0) {
		ocp_opt.ocp_cluster2_enable = 1;
		if (HW_API_DEBUG_ON)
			ocp_info("Cluster 2 enable.\n");
	return 0;
	}
} else {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Units != OCP_mA/mW (0/1)");
}

return -1;

}


void BigOCPDisable(void)
{
	spin_lock(&reset_lock);
	if (reset_flags == 0) {
		mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPDISABLE, 0, 0, 0);
		ocp_opt.ocp_cluster2_enable = 0;
		ocp_opt.ocp_cluster2_flag = 0;
		if (HW_API_DEBUG_ON)
			ocp_info("Cluster 2 disable.\n");
	}
	spin_unlock(&reset_lock);
}


int BigOCPIntLimit(int Select, int Limit)
{

const int TotMaxAct = 0;
const int TotMinAct = 1;
const int TotMaxPred = 2;
const int TotMinPred = 3;
const int TotLkgMax = 4;
const int ClkPctMin = 5;

if ((Select == TotLkgMax) || (Select == TotMaxAct) || (Select == TotMinAct)
	|| (Select == TotMaxPred) || (Select == TotMinPred)) {
	if ((Limit < 0) || (Limit > 127000)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Limit must be 0 ~ 127000 mA(mW)\n");

return -1;

} } else if (Select == ClkPctMin) {
	if ((Limit < 625) || (Limit > 10000)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Limit must be  625 (6.25%%)~ 10000 (100.00%%)\n");

		return -1;
	}
} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("Select is valid\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPINTLIMIT, Select, Limit, 0);

}


int BigOCPIntEnDis(int Value1, int Value0)
{
/* 1. Check Valid Value0 & Value1 (exit/return error -1 if invalid)
2. Write 32 bit value IRQEn to enable selected interrupt sources
*/

if ((Value1 < 0) || (Value1 > 255)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter must be 0 ~ 255\n");

return -1;
}
if ((Value0 < 0) || (Value0 > 255)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter must be 0 ~ 255\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPINTENDIS, Value1, Value0, 0);

}


int BigOCPIntClr(int Value1, int Value0)
{
/* 1. Check Valid Value0 & Value1 (exit/return error -1 if invalid)
2. Write 32 bit value IRQClr to clear selected bits
3. Then write all 0's
*/

if ((Value1 < 0) || (Value1 > 65535)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter must be 0 ~ 65535\n");

return -1;
}
if ((Value0 < 0) || (Value0 > 65535)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter must be 0 ~ 65535\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPINTCLR, Value1, Value0, 0);

}


void BigOCPIntStatus(int *Value1, int *Value0)
{
/* 1. Read IRQStatus registers & return IRQ1 at *Value1 & IRQ2 at *Value0	*/
int Temp;

if ((cpu_online(8) == 1) || (cpu_online(9) == 1)) {
	Temp = ocp_read(OCPAPBSTATUS00);
	*Value1 = (Temp & 0x00FF0000) >> 16;
	*Value0 = Temp & 0X000000FF;
	if (HW_API_DEBUG_ON)
		ocp_info("BIG IRQ1:IRQ0=%x:%x STAT00=%x\n", *Value1, *Value0, Temp);
	}
}


int BigOCPCapture(int EnDis, int Edge, int Count, int Trig)
{
int ret;
ret = 0;

if (ocp_opt.ocp_cluster2_enable == 0)
	return -1;

if (EnDis == OCP_DISABLE) {
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return -1;

spin_lock(&reset_lock);
if (reset_flags == 0)
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCAPTURE0, Edge, Count, Trig);
spin_unlock(&reset_lock);

if (ret == 0)
	return 0;
else
	return -1;

}
if (!((Edge == OCP_RISING) || (Edge == OCP_FALLING))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter OCP_RISING/OCP_FALLING must be 1/0\n");

return -2;
}

if ((Count < 0) || (Count > 1048575)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Count must be 0 ~ 1048575\n");

return -3;
}

if (!((Trig == OCP_CAP_IMM) || (Trig == OCP_CAP_IRQ0) || (Trig == OCP_CAP_IRQ1)
	|| (Trig == OCP_CAP_EO) || (Trig == OCP_CAP_EI))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter OCP_CAP_IMM/OCP_CAP_IRQ0/OCP_CAP_IRQ1/OCP_CAP_EO/OCP_CAP_EI must be 15/1/2/3/4\n");

return -4;
}

if ((EnDis == OCP_DISABLE) || (EnDis == OCP_ENABLE)) {
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return -1;

spin_lock(&reset_lock);
if (reset_flags == 0)
	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCAPTURE1, Edge, Count, Trig);
spin_unlock(&reset_lock);

if (ret == 0)
	return 0;
else
	return -1;

} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

}


int BigOCPCaptureStatus(int *Leakage, int *Total, int *ClkPct)
{
/*
1. Read first Capture status register
2. if valid=0, return all values as 0's and exit/return 0
3. else continue reading all Capture status
4. Convert all values to integer, return 1
*/
int Temp;

if (ocp_opt.ocp_cluster2_enable == 0)
	goto Label;
if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
	goto Label;

spin_lock(&reset_lock);
if (reset_flags == 0)
	Temp = ocp_read(OCPAPBSTATUS01);
else
	Temp = 0;
spin_unlock(&reset_lock);


if (GET_BITS_VAL_OCP(0:0, Temp) == 1) {

	/* CapOCCGPct -> 100.00% */
	*ClkPct = (10000*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;

	/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA) */
	*Leakage = (((Temp & 0x07FFFF00) >> 8) * 1000) >> 12;

	/* TotScaler */
	Temp = ocp_opt.ocp_cluster2_OCPAPBCFG24;
	if (GET_BITS_VAL_OCP(25:25, Temp) == 1)
		*Leakage = *Leakage >> (GET_BITS_VAL_OCP(2:0, ~GET_BITS_VAL_OCP(24:22, Temp)) + 1);
	else
		*Leakage = *Leakage << (GET_BITS_VAL_OCP(24:22, Temp));

	if (*Leakage > 127999)
		*Leakage = 127999;


if (ocp_opt.ocp_cluster2_enable == 0)
	goto Label;
if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
	goto Label;

	/* CapTotAct:  Q8.12 -> integer  */
	spin_lock(&reset_lock);
	if (reset_flags == 0)
		Temp = ocp_read(OCPAPBSTATUS02);
	else
		Temp = 0;
	spin_unlock(&reset_lock);


	*Total = ((Temp & 0x0007FFFF) * 1000) >> 12;
	if (HW_API_DEBUG_ON)
		ocp_info("ocp: big total_pwr=%d Lk_pwr=%d CG=%d\n", *Total, *Leakage, *ClkPct);

return 1;
} else {

Label:
	*Leakage = 0x0;
	*Total = 0x0;
	*ClkPct = 0x0;
return 0;
}
}


int BigOCPClkAvg(int EnDis, int CGAvgSel)
{

int ret;

if (!((EnDis == OCP_DISABLE) || (EnDis == OCP_ENABLE))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

if ((CGAvgSel < 0) || (CGAvgSel > 7)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter CGAvgSel must be 0 ~ 7\n");

return -1;
}

if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
	return -1;

ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCLKAVG, EnDis, CGAvgSel, 0);

return ret;

}

int BigOCPClkAvgStatus(unsigned int *CGAvg)
{
/*
1. Read first status register
2. if valid=0, return all values as 0's and exit/return 0
3. else continue reading status
*/
unsigned int Temp;

if ((cpu_online(8) == 0) && (cpu_online(9) == 0)) {
	*CGAvg = 0x0;
	return -1;
}
if (ocp_read_field(OCPAPBSTATUS04, 27:27) == 1) {
	Temp = ocp_read(OCPAPBSTATUS04);

/* CGAvg: shfit 11 bit -> UQ4.12  -> 100.00%  */
*CGAvg = ((((Temp & 0x07FFF800) >> 23) + 1) * 10000)/16;

} else {
*CGAvg = 0x0;

}
	if (HW_API_DEBUG_ON)
		ocp_info("ocp: big CGAvg=%d\n",  *CGAvg);

return 0;
}


int BigOCPCaptureRawLkgStatus(int *TopRawLkg, int *CPU0RawLkg, int *CPU1RawLkg)
{

int Temp;

if (ocp_read_field(OCPAPBSTATUS01, 0:0) == 1) {
	Temp = ocp_read(OCPAPBSTATUS04);

/* n bit * 1.5uA_x1000, Each bit represents ~1.5uA of current from the leakage sensor */
	*TopRawLkg = (Temp & 0x7FF);
	Temp = ocp_read(OCPAPBSTATUS05);
	/* n bit * 1.5uA_x1000 */
	*CPU0RawLkg = (Temp & 0x7FF);
	/* n bit * 1.5uA_x1000 */
	*CPU1RawLkg = ((Temp & 0x003FF800) >> 11);


	if (HW_API_DEBUG_ON)
		ocp_info("TopRawLkg=%d, CPU0RawLkg=%d, CPU1RawLkg=%d\n", *TopRawLkg, *CPU0RawLkg, *CPU1RawLkg);

return 1;
} else {
	*TopRawLkg = 0;
	*CPU0RawLkg = 0;
	*CPU1RawLkg = 0;
return 0;
}

}

int BigOCPMAFAct(unsigned int *CapMAFAct)
{

if (ocp_read_field(OCPAPBSTATUS01, 0:0) == 1)
	*CapMAFAct = (ocp_read_field(OCPAPBSTATUS03, 18:0) * 1000) >> 12; /* mA*/
else
	*CapMAFAct = 0x0;

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: big CapMAFAct=%d\n",  *CapMAFAct);

return 0;
}


void BigOCPAvgPwrGet_by_Little(void *info)
{

if ((cpu_online(8) == 1) || (cpu_online(9) == 1))
	*(unsigned int *)info = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPAVGPWRGET, *(unsigned int *)info, 0, 0);

}

unsigned int BigOCPAvgPwrGet(unsigned int Count)
{
void *info;

if (ocp_opt.ocp_cluster2_enable == 0)
	return 1;

if (Count < 0 || Count > 4294967295) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Count must be 0 ~ 4294967295");

return 1;
}

if (do_ocp_test_on == 1)
	return 1;

info = &Count;


if (smp_call_function_single(4, BigOCPAvgPwrGet_by_Little, info, 1) == 0)
	return Count;
if (smp_call_function_single(0, BigOCPAvgPwrGet_by_Little, info, 1) == 0)
	return Count;

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPAVGPWRGET, Count, 0, 0);

}



/* Little CPU */
int LittleOCPConfig(int Cluster, int VOffInmV, int VStepInuV)
{

if (Cluster == OCP_LL) {
	if (ocp_opt.ocp_cluster0_enable == 1)
		return 0;
}
if (Cluster == OCP_L) {
	if (ocp_opt.ocp_cluster1_enable == 1)
		return 0;
}


if (VOffInmV < 0 || VOffInmV > 1000) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VOffInmV must be 0 ~ 1000 mV");

return -1;
}
if (VStepInuV < 1000 || VStepInuV > 100000) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VStepInuV must be 1000 ~ 100000 uV");

return -1;
}

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPCONFIG, Cluster, VOffInmV, VStepInuV);

}


int LittleOCPSetTarget(int Cluster, int Target)
{

if ((Target < 0) || (Target > 127000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Target must be 0 ~ 127000 mA(mW)\n");

return -1;
}

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}
if (HW_API_DEBUG_ON)
	ocp_info("Set Cluster %d target limit = %d mW(mA)\n", Cluster, Target);

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPSETTARGET, Cluster, Target, 0);
}


int LittleOCPEnable(int Cluster, int Units, int ClkPctMin)
{
int ret;

if (!((Units == OCP_MA) || (Units == OCP_MW))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter OCP_MA/OCP_MW Units must be 0/1");

return -1;
}

if ((ClkPctMin < 625) || (ClkPctMin > 10000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter ClkPctMin must be  625 (6.25%%)~ 10000 (100.00%%)");

return -1;
}

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster must be 0/1");

return -1;
}

if (Cluster == OCP_LL) {
	if (ocp_opt.ocp_cluster0_enable == 1)
		return 0;

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPENABLE, Cluster, Units, ClkPctMin);
	if (ret == 0) {
		/* ocp_opt.ocp_cluster0_enable = 1; */
		if (HW_API_DEBUG_ON)
			ocp_info("Cluster 0 flag.\n");
	return 0;
	}
}
if (Cluster == OCP_L) {
	if (ocp_opt.ocp_cluster1_enable == 1)
		return 0;

	ret = mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPENABLE, Cluster, Units, ClkPctMin);
	if (ret == 0) {
		/* ocp_opt.ocp_cluster1_enable = 1;*/
		if (HW_API_DEBUG_ON)
			ocp_info("Cluster 1 flag.\n");
	return 0;
	}
}

return -1;

}


int LittleOCPDisable(int Cluster)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}
if (Cluster == OCP_LL) {
	ocp_opt.ocp_cluster0_enable = 0;
	ocp_opt.ocp_cluster0_flag = 0;
	if (HW_API_DEBUG_ON)
		ocp_info("Cluster 0 disable.\n");
}
if (Cluster == OCP_L) {
	ocp_opt.ocp_cluster1_enable = 0;
	ocp_opt.ocp_cluster1_flag = 0;
	if (HW_API_DEBUG_ON)
		ocp_info("Cluster 1 disable.\n");
}
return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPDISABLE, Cluster, 0, 0);

}


int LittleOCPDVFSSet(int Cluster, int FreqMHz, int VoltInmV)
{

if ((FreqMHz < 100) || (FreqMHz > 3000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter FreqMHz must be 100 ~ 3000 Mhz\n");

return -1;
}
if ((VoltInmV < 600) || (VoltInmV > 1300)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VoltInmV must be 600 ~ 1300 mV\n");

return -1;
}

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}
if (HW_API_DEBUG_ON)
	ocp_info("Set Cluster %d Freq= %d Mhz, Volt= %d mV\n", Cluster, FreqMHz, VoltInmV);

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPDVFSSET, Cluster, FreqMHz, VoltInmV);

}


int LittleOCPIntLimit(int Cluster, int Select, int Limit)
{
const int TotMaxAct = 0;
const int TotMinAct = 1;
const int TotLkgMax = 4;
const int ClkPctMin = 5;

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

if ((Select == TotLkgMax) || (Select == TotMaxAct) || (Select == TotMinAct)) {
	if ((Limit < 0) || (Limit > 127000)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Limit must be 0 ~ 127000 mA(mW)\n");

return -1;

} } else if (Select == ClkPctMin) {
	if ((Limit < 625) || (Limit > 10000)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Limit must be 625 (6.25%%)~ 10000 (100.00%%)\n");

return -1;

} } else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("Select is valid\n");

return -1;
}

if (HW_API_DEBUG_ON)
	ocp_info("Set Cluster %d Select= %d, Limit= %d\n", Cluster, Select, Limit);

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPINTLIMIT, Cluster, Select, Limit);


}


int LittleOCPIntEnDis(int Cluster, int Value1, int Value0)
{

if (Cluster == OCP_LL) {
	if ((Value1 < 0) || (Value1 > 255)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("input parameter must be 0 ~ 255\n");

return -1;
}
	if ((Value0 < 0) || (Value0 > 255)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("input parameter must be 0 ~ 255\n");

	return -1;
}

} else if (Cluster == OCP_L) {
	if ((Value1 < 0) || (Value1 > 255)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("input parameter must be 0 ~ 255\n");

return -1;
}
if ((Value0 < 0) || (Value0 > 255)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("input parameter must be 0 ~ 255\n");

return -1;
}

} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPINTENDIS, Cluster, Value1, Value0);

}


int LittleOCPIntClr(int Cluster, int Value1, int Value0)
{

if (Cluster == OCP_LL) {
	if ((Value1 < 0) || (Value1 > 65535)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("input parameter must be 0 ~ 65535\n");

return -1;
}
if ((Value0 < 0) || (Value0 > 65535)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("input parameter must be 0 ~ 65535\n");

return -1;
}

} else if (Cluster == OCP_L) {
	if ((Value1 < 0) || (Value1 > 65535)) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("input parameter must be 0 ~ 65535\n");

return -1;
}
if ((Value0 < 0) || (Value0 > 65535)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("input parameter must be 0 ~ 65535\n");

return -1;
}
} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}
return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPINTCLR, Cluster, Value1, Value0);
}


int LittleOCPIntStatus(int Cluster, int *Value1, int *Value0)
{
/* 0. Apply to selected cluster L or LL
1. Read IRQStatus registers & return IRQ1 at *Value1 & IRQ2 at *Value0
*/
int Temp;
if (Cluster == OCP_LL) {
	Temp = ocp_read(MP0_OCP_IRQSTATE);
	*Value1 = (Temp & 0x00FF0000) >> 16;
	*Value0 = Temp & 0X000000FF;
	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL0 IRQ1:IRQ0=%x:%x IRQSTATE=%x\n", *Value1, *Value0, ocp_read(MP0_OCP_IRQSTATE));

} else if (Cluster == OCP_L) {
	Temp = ocp_read(MP1_OCP_IRQSTATE);
	*Value1 = (Temp & 0x00FF0000) >> 16;
	*Value0 = Temp & 0X000000FF;
	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL1 IRQ1:IRQ0=%x:%x IRQSTATE=%x\n", *Value1, *Value0, ocp_read(MP1_OCP_IRQSTATE));

} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

return 0;
}


int LittleOCPCapture(int Cluster, int EnDis, int Edge, int Count, int Trig)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

if (EnDis == OCP_DISABLE) {
	return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPCAPTURE00, Cluster, Count, Trig);
} else if (EnDis != OCP_ENABLE) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

if ((Count < 0) || (Count > 1048575)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Count must be 0 ~ 1048575\n");

return -3;
}

if (!((Trig == OCP_CAP_IMM) || (Trig == OCP_CAP_IRQ0) || (Trig == OCP_CAP_IRQ1)
	|| (Trig == OCP_CAP_EO) || (Trig == OCP_CAP_EI))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter OCP_CAP_IMM/OCP_CAP_IRQ0/OCP_CAP_IRQ1/OCP_CAP_EO/OCP_CAP_EI must be 15/1/2/3/4\n");

return -4;
}

if (Edge == OCP_RISING) {
	return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPCAPTURE11, Cluster, Count, Trig);
} else if (Edge == OCP_FALLING) {
	return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPCAPTURE10, Cluster, Count, Trig);
} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter OCP_RISING/OCP_FALLING must be 1/0\n");

return -2;
}

}


int LittleOCPCaptureGet(int Cluster, int *Leakage, int *Total, int *ClkPct)
{
/*
0. Apply to selected cluster L or LL
1. Read first Capture status register
2. if valid=0, return all values as 0's and exit/return 0
3. else continue reading all Capture status
4. Convert all values to integer, return 1
*/
int Temp;
if (Cluster == OCP_LL) {
	if (ocp_read_field((MP0_OCP_CAP_STATUS00), 0:0) == 1) {
		Temp = ocp_read((MP0_OCP_CAP_STATUS00));
		/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA) */
		*Leakage = (((Temp & 0x07FFFF00) >> 8) * 1000) >> 12;

		/* TotScaler */
		if (ocp_read_field(MP0_OCP_SCAL, 3:3) == 1)
			*Leakage = *Leakage >> (GET_BITS_VAL_OCP(2:0, ~ocp_read_field(MP0_OCP_SCAL, 2:0)) + 1);
		else
			*Leakage = *Leakage << (ocp_read_field(MP0_OCP_SCAL, 2:0));

		if (*Leakage > 127999)
			*Leakage = 127999;

		/* CapOCCGPct -> 100.00% */
		*ClkPct = (10000*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;
		Temp = ocp_read((MP0_OCP_CAP_STATUS01));
		/* CapTotAct:  Q8.12 -> integer (mA) */
		*Total = ((Temp & 0x0007FFFF) * 1000) >> 12;

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL0 total_pwr=%d Lk_pwr=%d CG=%d\n", *Total, *Leakage, *ClkPct);

return 1;
	} else {
		*Leakage = 0x0;
		*Total = 0x0;
		*ClkPct = 0x0;
		return 0;
	}

} else if (Cluster == OCP_L) {
	if (ocp_read_field((MP1_OCP_CAP_STATUS00), 0:0) == 1) {
		Temp = ocp_read((MP1_OCP_CAP_STATUS00));
		/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA) */
		*Leakage = (((Temp & 0x07FFFF00) >> 8) * 1000) >> 12;

		/* TotScaler */
		if (ocp_read_field(MP1_OCP_SCAL, 3:3) == 1)
			*Leakage = *Leakage >> (GET_BITS_VAL_OCP(2:0, ~ocp_read_field(MP1_OCP_SCAL, 2:0)) + 1);
		else
			*Leakage = *Leakage << (ocp_read_field(MP1_OCP_SCAL, 2:0));

		if (*Leakage > 127999)
			*Leakage = 127999;

	/* CapOCCGPct -> 100.00% */
		*ClkPct = (10000*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;
		Temp = ocp_read((MP1_OCP_CAP_STATUS01));
		/* CapTotAct:  Q8.12 -> integer (mA, mW) */
		*Total = ((Temp & 0x0007FFFF) * 1000) >> 12;

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL1 total_pwr=%d Lk_pwr=%d CG=%d\n", *Total, *Leakage, *ClkPct);

return 1;
	} else {
		*Leakage = 0x0;
		*Total = 0x0;
		*ClkPct = 0x0;
		return 0;
}

} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

		return -1;
}

}

int CL0OCPCaptureRawLkgStatus(int *TopRawLkg, int *CPU0RawLkg, int *CPU1RawLkg, int *CPU2RawLkg, int *CPU3RawLkg)
{
	if (ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0) == 1) {
		/* n bit * 1.5uA */
	*TopRawLkg = (ocp_read_field(MP0_OCP_CAP_STATUS03, 10:0));
	*CPU0RawLkg = (ocp_read_field(MP0_OCP_CAP_STATUS04, 10:0));
	*CPU1RawLkg = (ocp_read_field(MP0_OCP_CAP_STATUS04, 21:11));
	*CPU2RawLkg = (ocp_read_field(MP0_OCP_CAP_STATUS05, 10:0));
	*CPU3RawLkg = (ocp_read_field(MP0_OCP_CAP_STATUS05, 21:11));

	if (HW_API_DEBUG_ON)
		ocp_info("cluster 0: TopRawLkg=%d, CPU0RawLkg=%d, CPU1RawLkg=%d, CPU2RawLkg=%d, CPU3RawLkg=%d\n",
		*TopRawLkg, *CPU0RawLkg, *CPU1RawLkg, *CPU2RawLkg, *CPU3RawLkg);

return 1;
} else {
	*TopRawLkg = 0;
	*CPU0RawLkg = 0;
	*CPU1RawLkg = 0;
	return 0;
}

}

int CL1OCPCaptureRawLkgStatus(int *TopRawLkg, int *CPU0RawLkg, int *CPU1RawLkg, int *CPU2RawLkg, int *CPU3RawLkg)
{
	if (ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0) == 1) {
		/* n bit * 1.5uA */
	*TopRawLkg = (ocp_read_field(MP1_OCP_CAP_STATUS03, 10:0));
	*CPU0RawLkg = (ocp_read_field(MP1_OCP_CAP_STATUS04, 10:0));
	*CPU1RawLkg = (ocp_read_field(MP1_OCP_CAP_STATUS04, 21:11));
	*CPU2RawLkg = (ocp_read_field(MP1_OCP_CAP_STATUS05, 10:0));
	*CPU3RawLkg = (ocp_read_field(MP1_OCP_CAP_STATUS05, 21:11));

	if (HW_API_DEBUG_ON)
		ocp_info("cluster 1: TopRawLkg=%d, CPU0RawLkg=%d, CPU1RawLkg=%d, CPU2RawLkg=%d, CPU3RawLkg=%d\n",
		*TopRawLkg, *CPU0RawLkg, *CPU1RawLkg, *CPU2RawLkg, *CPU3RawLkg);

return 1;
} else {
	*TopRawLkg = 0;
	*CPU0RawLkg = 0;
	*CPU1RawLkg = 0;
	return 0;
}

}

int LittleLowerPowerOn(int Cluster, int Test_bit)
{
if (Cluster == OCP_LL) {
	ocp_write_field(MP0_OCP_ENABLE, 0:0, 0);
	ocp_write(MP0_OCP_GENERAL_CTRL, 0x0);
	ocp_opt.ocp_cluster0_enable = 0;
} else if (Cluster == OCP_L) {
	ocp_write_field(MP1_OCP_ENABLE, 0:0, 0);
	ocp_write(MP1_OCP_GENERAL_CTRL, 0x0);
	ocp_opt.ocp_cluster1_enable = 0;
} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");
	return -1;
}
	do_ocp_test_on = Test_bit;

return 0;
}

int LittleLowerPowerOff(int Cluster, int Test_bit)
{
if (Cluster == OCP_LL) {
	ocp_write(MP0_OCP_GENERAL_CTRL, 0x6);
	ocp_write_field(MP0_OCP_ENABLE, 0:0, 1);
	ocp_opt.ocp_cluster0_enable = 1;
} else if (Cluster == OCP_L) {
	ocp_write(MP1_OCP_GENERAL_CTRL, 0x6);
	ocp_write_field(MP1_OCP_ENABLE, 0:0, 1);
	ocp_opt.ocp_cluster1_enable = 1;
} else {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");
	return -1;
}
	do_ocp_test_on = Test_bit;

return 0;
}

/* lower power for BU */
int LittleOCPAvgPwr(int Cluster, int EnDis, int Count)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

if (!((EnDis == OCP_DISABLE) || (EnDis == OCP_ENABLE))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

if ((Count < 0) || (Count > 4194304)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Count must be 0 ~ 4194304\n");

return -1;
}

if (do_ocp_test_on == 1)
	return 0;

if (Cluster == OCP_LL)
	ocp_opt.ocp_cluster0_enable = 1;
else if (Cluster == OCP_L)
	ocp_opt.ocp_cluster1_enable = 1;

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPAVGPWR, Cluster, EnDis, Count);
}

/* lower power for BU */
unsigned int LittleOCPAvgPwrGet(int Cluster)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return 0;
}

if (do_ocp_test_on == 1)
	return 1;

if (Cluster == OCP_LL)
	ocp_opt.ocp_cluster0_enable = 0;
else if (Cluster == OCP_L)
	ocp_opt.ocp_cluster1_enable = 0;

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPAVGPWRGET, Cluster, 0, 0);

}

/* for DVT, HQA only */
int LittleOCPAvgPwr_HQA(int Cluster, int EnDis, int Count)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

if (!((EnDis == OCP_DISABLE) || (EnDis == OCP_ENABLE))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

if ((Count < 0) || (Count > 4194304)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Count must be 0 ~ 4194304\n");

return -1;
}

if (Cluster == OCP_LL) {
	/*ocp_write_field(MP0_OCP_GENERAL_CTRL, 1:1, EnDis);*/
	ocp_write_field(MP0_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP0_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP0_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP0_OCP_DBG_STAT, 0:0));
	}
else if (Cluster == OCP_L) {
	/*ocp_write_field(MP1_OCP_GENERAL_CTRL, 1:1, EnDis);*/
	ocp_write_field(MP1_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP1_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP1_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP1_OCP_DBG_STAT, 0:0));
}

return 0;
}

/* for DVT, HQA only */
int LittleOCPAvgPwrGet_HQA(int Cluster, unsigned long long *AvgLkg, unsigned long long *AvgAct)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
}

if (Cluster == OCP_LL) {
	if (ocp_read_field(MP0_OCP_DBG_STAT, 31:31) == 1) {
				*AvgLkg = ((((((unsigned long long)ocp_read(MP0_OCP_DBG_LKG_H) << 32) +
				(unsigned long long)ocp_read(MP0_OCP_DBG_LKG_L)) * 32) /
				(unsigned long long)ocp_read_field(MP0_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

				/* TotScaler */
				if (ocp_read_field(MP0_OCP_SCAL, 3:3) == 1)
					*AvgLkg = *AvgLkg >> (GET_BITS_VAL_OCP(2:0,
								~ocp_read_field(MP0_OCP_SCAL, 2:0)) + 1);
				else
					*AvgLkg = *AvgLkg << (ocp_read_field(MP0_OCP_SCAL, 2:0));

				*AvgAct = ((((((unsigned long long)ocp_read(MP0_OCP_DBG_ACT_H) << 32) +
				(unsigned long long)ocp_read(MP0_OCP_DBG_ACT_L)) * 32) /
				(unsigned long long)ocp_read_field(MP0_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

	} else {
		*AvgLkg = 0;
		*AvgAct = 0;
	}

} else if (Cluster == OCP_L) {
	if (ocp_read_field(MP1_OCP_DBG_STAT, 31:31) == 1) {
				*AvgLkg = ((((((unsigned long long)ocp_read(MP1_OCP_DBG_LKG_H) << 32) +
				(unsigned long long)ocp_read(MP1_OCP_DBG_LKG_L)) * 32) /
				(unsigned long long)ocp_read_field(MP1_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

				/* TotScaler */
				if (ocp_read_field(MP1_OCP_SCAL, 3:3) == 1)
					*AvgLkg = *AvgLkg >> (GET_BITS_VAL_OCP(2:0,
								~ocp_read_field(MP1_OCP_SCAL, 2:0)) + 1);
				else
					*AvgLkg = *AvgLkg << (ocp_read_field(MP1_OCP_SCAL, 2:0));

				*AvgAct = ((((((unsigned long long)ocp_read(MP1_OCP_DBG_ACT_H) << 32) +
				(unsigned long long)ocp_read(MP1_OCP_DBG_ACT_L)) * 32) /
				(unsigned long long)ocp_read_field(MP1_OCP_DBG_IFCTRL1, 21:0)) * 1000) >> 12;

	} else {
		*AvgLkg = 0;
		*AvgAct = 0;
	}

}

return 0;
}

int LittleOCPMAFAct(int Cluster, unsigned int *CapMAFAct)
{
if (Cluster == OCP_LL) {
	if (ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0) == 1) {
		*CapMAFAct = (ocp_read_field(MP0_OCP_CAP_STATUS02, 18:0) * 1000) >> 12; /* mA*/

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL0 MAFAct=%d\n", *CapMAFAct);

	} else {
		*CapMAFAct = 0x0;

	}

} else if (Cluster == OCP_L) {
	if (ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0) == 1) {
		*CapMAFAct = (ocp_read_field(MP1_OCP_CAP_STATUS02, 18:0) * 1000) >> 12; /* mA*/

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL1 MAFAct=%d\n", *CapMAFAct);

	} else {
		*CapMAFAct = 0x0;

	}
	}
return 0;
}

int OCPMcusysPwrSet(void)
{
	ocp_write(CCI_BW_PMU_CNT0to1_SEL, 0x9E809E00);
	ocp_write(CCI_BW_PMU_CNT2to3_SEL, 0xAE80AE00);
	ocp_write_field(CCI_BW_PMU_CTL, 0:0, 0x1);
return 0;
}

int OCPMcusysPwrCapture(unsigned int *cnt, unsigned int *cnt0, unsigned int *cnt1,
					unsigned int *cnt2, unsigned int *cnt3)
{
	ocp_write_field(CCI_BW_PMU_CTL, 0:0, 0x0);
	*cnt = ocp_read(CCI_BW_PMU_REF_CNT);
	*cnt0 = ocp_read(CCI_BW_PMU_ACC_CNT0);
	*cnt1 = ocp_read(CCI_BW_PMU_ACC_CNT1);
	*cnt2 = ocp_read(CCI_BW_PMU_ACC_CNT2);
	*cnt3 = ocp_read(CCI_BW_PMU_ACC_CNT3);
return 0;
}

unsigned int OCPMcusysPwrGet(void)
{

	/* mcusys power*/
	return 207;
}


/* Big OCP init API */
void Cluster2_OCP_ON(void)
{
if (BIG_OCP_ON) {
	BigOCPConfig(300, 10000);
	BigOCPSetTarget(3, 127000);
	BigOCPEnable(3, 1, 625, 0);
	ocp_opt.ocp_cluster2_flag = 1;
	ocp_opt.ocp_cluster2_enable = 1;
	spin_lock(&reset_lock);
	if (reset_flags == 0)
		ocp_opt.ocp_cluster2_OCPAPBCFG24 = ocp_read(OCPAPBCFG24);
	spin_unlock(&reset_lock);
#if defined(CONFIG_OCP_AEE_RR_REC) && !defined(EARLY_PORTING)
	aee_rr_rec_ocp_2_enable(1);
#endif
	BigiDVFSChannel(1, 1);
	}
}

void Cluster2_OCP_OFF(void)
{
if (BIG_OCP_ON) {
	BigiDVFSChannel(1, 0);
#if defined(CONFIG_OCP_AEE_RR_REC) && !defined(EARLY_PORTING)
	aee_rr_rec_ocp_2_enable(0);
#endif
	ocp_opt.ocp_cluster2_flag = 0;
	ocp_opt.ocp_cluster2_enable = 0;
	BigOCPDisable();
	}
}

/* Little OCP init API */
void Cluster0_OCP_ON(void)
{
if (LITTLE_OCP_ON) {
	LittleOCPConfig(0, 300, 10000);
	LittleOCPSetTarget(0, 127000);
	LittleOCPDVFSSet(0, 624, 900);
	LittleOCPEnable(0, 1, 625);
	ocp_opt.ocp_cluster0_flag = 1;
	ocp_opt.ocp_cluster0_enable = 0;
	}
}

void Cluster0_OCP_OFF(void)
{
if (LITTLE_OCP_ON) {
	ocp_opt.ocp_cluster0_flag = 0;
	ocp_opt.ocp_cluster0_enable = 0;
	LittleOCPDisable(0);
	}
}

void Cluster1_OCP_ON(void)
{
if (LITTLE_OCP_ON) {
	LittleOCPConfig(1, 300, 10000);
	LittleOCPSetTarget(1, 127000);
	LittleOCPDVFSSet(1, 338, 780);
	LittleOCPEnable(1, 1, 625);
	ocp_opt.ocp_cluster1_flag = 1;
	ocp_opt.ocp_cluster1_enable = 0;
	}
}

void Cluster1_OCP_OFF(void)
{
if (LITTLE_OCP_ON) {
	ocp_opt.ocp_cluster1_flag = 0;
	ocp_opt.ocp_cluster1_enable = 0;
	LittleOCPDisable(1);
	}
}

int ocp_status_get(int cluster)
{
if (cluster == OCP_LL)
	return ocp_opt.ocp_cluster0_flag;
else if (cluster == OCP_L)
	return ocp_opt.ocp_cluster1_flag;
else if (cluster == OCP_B)
	return ocp_opt.ocp_cluster2_flag;
else
	return -1;
}



/* Cluster 2 DREQ */

int BigSRAMLDOEnable(int mVolts)
{
if ((mVolts < 500) || (mVolts > 1200)) {
	ocp_err("Error: SRAM LDO volte = %dmv, out of range 500mv~1200mv.\n", mVolts);
	/* out of range  */
return -1;
}
return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGSRAMLDOENABLE, mVolts, 0, 0);

}

int BigDREQHWEn(int VthHi, int VthLo)
{
int ret;

if (VthHi < VthLo) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter must VthHi >= VthLo\n");

return -1;
}
if ((VthHi < 600) || (VthHi > 1200)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VthHi must be 600 ~ 1200 mV.\n");

return -2;
}
if ((VthLo < 600) || (VthLo > 1200)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter VthLo must be 600 ~ 1200 mV.\n");

return -2;
}

ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGDREQHWEN, VthHi, VthLo, 0);

if (ret == 0)
	big_dreq_enable = 1;
else
	big_dreq_enable = 0;

return ret;

}


int BigDREQSWEn(int Value)
{

if ((Value < 0) || (Value > 1)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

if (Value == 1)
	big_dreq_enable = 1;
else
	big_dreq_enable = 0;

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGDREQSWEN, Value, 0, 0);

}

int BigDREQGet(void)
{
return  mt_secure_call_ocp(MTK_SIP_KERNEL_BIGDREQGET, 0, 0, 0);

}


/* Cluster 0,1 DREQ */
int LittleDREQSWEn(int EnDis)
{
if ((EnDis < 0) || (EnDis > 1)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

return  mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEDREQSWEN, EnDis, 0, 0);
}

int LittleDREQGet(void)
{

return  mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEDREQGET, 0, 0, 0);

}


/* OCP ISR Handler */
static irqreturn_t ocp_isr_cluster2(int irq, void *dev_id)
{
int ret = 0, IRQ1, IRQ0, val[3];

BigOCPIntStatus(&IRQ1, &IRQ0);
BigOCPCapture(1, 1, 0, 15);

if (irq == ocp2_irq1_number) {
	BigOCPIntClr(IRQ1, 0);

if (GET_BITS_VAL_OCP(4:4, IRQ1) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapToLkg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: TotLkg > LkgMax\n");

}
if (GET_BITS_VAL_OCP(7:7, IRQ1) == 1) {
	BigOCPClkAvgStatus(&val[0]);
	ocp_status[2].CGAvg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: CGAvg < OCCGMin\n");

}
if (GET_BITS_VAL_OCP(6:6, IRQ1) == 1) {
	ocp_status[2].CaptureValid = 1;

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: CaptureValid = 1\n");

}
if (GET_BITS_VAL_OCP(0:0, IRQ1) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: WAValAct > WAMaxAct\n");

}
if (GET_BITS_VAL_OCP(1:1, IRQ1) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: WAValAct < WAMinAct\n");

}
if (GET_BITS_VAL_OCP(5:5, IRQ1) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapOCCGPct = val[2];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: OCCGVal < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(2:2, IRQ1) == 1) {
	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: WAValPred > WAMaxPred\n");
}
if (GET_BITS_VAL_OCP(3:3, IRQ1) == 1) {
	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ1: WAValPred < WAMinPred\n");
}

if (IRQ_Debug_on)
	ocp_info("Cluster 2 IRQ1: %x\n", IRQ1);

ocp_status[2].IRQ1 = IRQ1;


}	else if (irq == ocp2_irq0_number)	{
	BigOCPIntClr(0, IRQ0);

if (GET_BITS_VAL_OCP(4:4, IRQ0) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapToLkg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: TotLkg > LkgMax\n");
}
if (GET_BITS_VAL_OCP(7:7, IRQ0) == 1) {
	BigOCPClkAvgStatus(&val[0]);
	ocp_status[2].CGAvg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: CGAvg < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(6:6, IRQ0) == 1) {
	ocp_status[2].CaptureValid = 1;

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: CaptureValid = 1\n");
}
if (GET_BITS_VAL_OCP(0:0, IRQ0) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: WAValAct > WAMaxAct\n");
}
if (GET_BITS_VAL_OCP(1:1, IRQ0) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: WAValAct < WAMinAct\n");
}
if (GET_BITS_VAL_OCP(5:5, IRQ0) == 1) {
	BigOCPCaptureStatus(&val[0], &val[1], &val[2]);
	ocp_status[2].CapOCCGPct = val[2];

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: OCCGVal < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(2:2, IRQ0) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: WAValPred > WAMaxPred\n");
}
if (GET_BITS_VAL_OCP(3:3, IRQ0) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 2 IRQ0: WAValPred < WAMinPred\n");
}

if (IRQ_Debug_on)
	ocp_info("Cluster 2 IRQ0: %x\n", IRQ0);

ocp_status[2].IRQ0 = IRQ0;
}
/* disable Cluster 2 IRQ */
ret = BigOCPIntEnDis(0, 0);

if (ret != 0)
	ocp_err("Cluster 2 invalid interrupt parameters.\n");

return IRQ_HANDLED;
}

static irqreturn_t ocp_isr_cluster0(int irq, void *dev_id)
{
int ret = 0, IRQ1, IRQ0, val[3];

LittleOCPIntStatus(0, &IRQ1, &IRQ0);
LittleOCPCapture(0, 1, 1, 0, 15);

if (irq == ocp0_irq1_number) {
	LittleOCPIntClr(0, IRQ1, 0);

if (GET_BITS_VAL_OCP(4:4, IRQ1) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapToLkg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: TotLkg > LkgMax\n");
}
if (GET_BITS_VAL_OCP(7:7, IRQ1) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: CGAvg < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(6:6, IRQ1) == 1) {
	ocp_status[0].CaptureValid = 1;

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: CaptureValid = 1\n");

}
if (GET_BITS_VAL_OCP(0:0, IRQ1) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: WAValAct > WAMaxAct\n");
}
if (GET_BITS_VAL_OCP(1:1, IRQ1) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: WAValAct < WAMinAct\n");
}
if (GET_BITS_VAL_OCP(5:5, IRQ1) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapOCCGPct = val[2];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: OCCGVal < OCCGMin\n");
}

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ1: %x\n", IRQ1);

ocp_status[0].IRQ1 = IRQ1;

} else if (irq == ocp0_irq0_number) {
	LittleOCPIntClr(0, 0, IRQ0);

if (GET_BITS_VAL_OCP(4:4, IRQ0) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapToLkg = val[0];

if (IRQ_Debug_on)
	ocp_info("Cluster 0 IRQ0: TotLkg > LkgMax\n");
}
if (GET_BITS_VAL_OCP(7:7, IRQ0) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: CGAvg < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(6:6, IRQ0) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: CaptureValid = 1\n");
}
if (GET_BITS_VAL_OCP(0:0, IRQ0) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: WAValAct > WAMaxAct\n");
}
if (GET_BITS_VAL_OCP(1:1, IRQ0) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: WAValAct < WAMinAct\n");
}
if (GET_BITS_VAL_OCP(5:5, IRQ0) == 1) {
	LittleOCPCaptureGet(0, &val[0], &val[1], &val[2]);
	ocp_status[0].CapOCCGPct = val[2];

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: OCCGVal < OCCGMin\n");
}

	if (IRQ_Debug_on)
		ocp_info("Cluster 0 IRQ0: %x\n", IRQ0);

ocp_status[0].IRQ0 = IRQ0;

}
/* disable Cluster 0 IRQ */
ret = LittleOCPIntEnDis(0, 0, 0);

if (ret != 0)
	ocp_err("Cluster 0 invalid interrupt parameters.\n");

return IRQ_HANDLED;
}

static irqreturn_t ocp_isr_cluster1(int irq, void *dev_id)
{
int ret = 0, IRQ1, IRQ0, val[3];



LittleOCPIntStatus(1, &IRQ1, &IRQ0);
LittleOCPCapture(1, 1, 1, 0, 15);

if (irq == ocp1_irq1_number) {
	LittleOCPIntClr(1, IRQ1, 0);

if (GET_BITS_VAL_OCP(4:4, IRQ1) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapToLkg = val[0];

	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ1: TotLkg > LkgMax\n");
}
if (GET_BITS_VAL_OCP(7:7, IRQ1) == 1) {

	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ1: CGAvg < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(6:6, IRQ1) == 1) {
	ocp_status[1].CaptureValid = 1;

	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ1: CaptureValid = 1\n");
}
if (GET_BITS_VAL_OCP(0:0, IRQ1) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ1: WAValAct > WAMaxAct\n");
}
if (GET_BITS_VAL_OCP(1:1, IRQ1) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapTotAct = val[1];

	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ1: WAValAct < WAMinAct\n");
}
if (GET_BITS_VAL_OCP(5:5, IRQ1) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapOCCGPct = val[2];


if (IRQ_Debug_on)
	ocp_info("Cluster 1 IRQ1: OCCGVal < OCCGMin\n");
}

if (IRQ_Debug_on)
	ocp_info("Cluster 1 IRQ1: %x\n", IRQ1);
ocp_status[1].IRQ1 = IRQ1;

} else if (irq == ocp1_irq0_number) {
	LittleOCPIntClr(1, 0, IRQ0);

if (GET_BITS_VAL_OCP(4:4, IRQ0) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapToLkg = val[0];
	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ0: TotLkg > LkgMax\n");
}
if (GET_BITS_VAL_OCP(7:7, IRQ0) == 1) {
		if (IRQ_Debug_on)
			ocp_info("Cluster 1 IRQ0: CGAvg < OCCGMin\n");
}
if (GET_BITS_VAL_OCP(6:6, IRQ0) == 1) {
	ocp_status[1].CaptureValid = 1;
	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ0: CaptureValid = 1\n");
}
if (GET_BITS_VAL_OCP(0:0, IRQ0) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapTotAct = val[1];

if (IRQ_Debug_on)
	ocp_info("Cluster 1 IRQ0: WAValAct > WAMaxAct\n");
}
if (GET_BITS_VAL_OCP(1:1, IRQ0) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapTotAct = val[1];
	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ0: WAValAct < WAMinAct\n");
}
if (GET_BITS_VAL_OCP(5:5, IRQ0) == 1) {
	LittleOCPCaptureGet(1, &val[0], &val[1], &val[2]);
	ocp_status[1].CapOCCGPct = val[2];
	if (IRQ_Debug_on)
		ocp_info("Cluster 1 IRQ0: OCCGVal < OCCGMin\n");
}

if (IRQ_Debug_on)
	ocp_info("Cluster 1 IRQ0: %x\n", IRQ0);

ocp_status[1].IRQ0 = IRQ0;
}
/* disable Cluster 1 IRQ */
ret = LittleOCPIntEnDis(1, 0, 0);

if (ret != 0)
	ocp_err("Cluster 1 invalid interrupt parameters.\n");


return IRQ_HANDLED;
}

/* Device infrastructure */
static int ocp_remove(struct platform_device *pdev)
{
return 0;
}

static int ocp_probe(struct platform_device *pdev)
{
int err = 0;

/* set OCP IRQ */
err = request_irq(ocp2_irq0_number, ocp_isr_cluster2, IRQF_TRIGGER_HIGH, "ocp_cluster2", NULL);

if (err) {
	ocp_err("OCP IRQ register failed: ocp2_irq0 (%d)\n", err);
	WARN_ON(1);
}
err = request_irq(ocp2_irq1_number, ocp_isr_cluster2, IRQF_TRIGGER_HIGH, "ocp_cluster2", NULL);

if (err) {
	ocp_err("OCP IRQ register failed: ocp2_irq1 (%d)\n", err);
	WARN_ON(1);
}

if (LITTLE_OCP_ON) {

	err = request_irq(ocp0_irq0_number, ocp_isr_cluster0, IRQF_TRIGGER_HIGH, "ocp_cluster0", NULL);

if (err) {
	ocp_err("OCP IRQ register failed: ocp0_irq0 (%d)\n", err);
	WARN_ON(1);
}

err = request_irq(ocp0_irq1_number, ocp_isr_cluster0, IRQF_TRIGGER_HIGH, "ocp_cluster0", NULL);

if (err) {
	ocp_err("OCP IRQ register failed: ocp0_irq1 (%d)\n", err);
	WARN_ON(1);
}

err = request_irq(ocp1_irq0_number, ocp_isr_cluster1, IRQF_TRIGGER_HIGH, "ocp_cluster1", NULL);

if (err) {
	ocp_err("OCP IRQ register failed : ocp1_irq0 (%d)\n", err);
	WARN_ON(1);
}

err = request_irq(ocp1_irq1_number, ocp_isr_cluster1, IRQF_TRIGGER_HIGH, "ocp_cluster1", NULL);

if (err) {
	ocp_err("OCP IRQ register failed  ocp1_irq1 (%d)\n", err);
	WARN_ON(1);
}
}
ocp_info("OCP Initial.\n");

#if defined(CONFIG_OCP_AEE_RR_REC) && !defined(EARLY_PORTING)
	_mt_ocp_aee_init();
	ocp_info("OCP ram console init.\n");
#endif


return 0;
}


static int ocp_suspend(struct platform_device *pdev, pm_message_t state)
{
/*
kthread_stop(ocp_thread);
*/

ocp_info("OCP suspend\n");
return 0;
}


static int ocp_resume(struct platform_device *pdev)
{
/*
ocp_thread = kthread_run(ocp_thread_handler, 0, "ocp xxx");
if (IS_ERR(ocp_thread))
{
printk("[%s]: failed to create ocp xxx thread\n", __func__);
}
*/

ocp_info("OCP resume\n");
return 0;
}

struct platform_device ocp_pdev = {
	.name   = "mt_ocp",
	.id     = -1,
};

static struct platform_driver ocp_pdrv = {
	.remove     = ocp_remove,
	.shutdown   = NULL,
	.probe      = ocp_probe,
	.suspend    = ocp_suspend,
	.resume     = ocp_resume,
	.driver     = {
	.name   = "mt_ocp",
	},
};


#ifdef CONFIG_PROC_FS

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


/* ocp_enable*/
static int ocp_cluster2_enable_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 2 OCP flag = %d\n", ocp_opt.ocp_cluster2_flag);
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_opt.ocp_cluster2_enable);
return 0;
}

static ssize_t ocp_cluster2_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[5];
char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) > 0) {
	switch (function_id) {
	case 8:
			ocp_opt.ocp_cluster2_flag = 0;
			BigOCPConfig(300, 10000);
			BigOCPSetTarget(3, 127000);
			BigOCPEnable(3, 1, 625, 0);
			ocp_opt.ocp_cluster2_OCPAPBCFG24 = ocp_read(OCPAPBCFG24);
		break;
	case 3:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			BigOCPConfig(val[0], val[1]);
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			BigOCPSetTarget(val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5) {
			BigOCPEnable(val[0], val[1], val[2], val[3]);
			ocp_opt.ocp_cluster2_OCPAPBCFG24 = ocp_read(OCPAPBCFG24);
		}
		break;
	case 0:
		BigOCPDisable();
		ocp_status[2].IntEnDis = 0;
		ocp_status[2].IRQ1 = 0;
		ocp_status[2].IRQ0 = 0;
		ocp_status[2].CapToLkg = 0;
		ocp_status[2].CapOCCGPct = 0;
		ocp_status[2].CaptureValid = 0;
		ocp_status[2].CapTotAct = 0;
		ocp_status[2].CapMAFAct = 0;
		ocp_status[2].CGAvgValid = 0;
		ocp_status[2].CGAvg = 0;
		ocp_status[2].AvgLkg = 0;
		ocp_status[2].TopRawLkg = 0;
		ocp_status[2].CPU0RawLkg = 0;
		ocp_status[2].CPU1RawLkg = 0;
		ocp_status[2].CPU2RawLkg = 0;
		ocp_status[2].CPU3RawLkg = 0;
		ocp_info("Cluster 2 Big OCP Disable\n");


		break;

	default:
		ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

static int ocp_cluster0_enable_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 0 OCP flag = %d\n", ocp_opt.ocp_cluster0_flag);
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_opt.ocp_cluster0_enable);
return 0;
}

static ssize_t ocp_cluster0_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[3];
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
	switch (function_id) {
	case 8:
		ocp_opt.ocp_cluster0_flag = 0;
		LittleOCPConfig(0, 300, 10000);
		LittleOCPSetTarget(0, 127000);
		LittleOCPDVFSSet(0, 624, 900);
		LittleOCPEnable(0, 1, 625);
		break;
	case 4:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			LittleOCPConfig(0, val[0], val[1]);
		break;
	case 3:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			LittleOCPSetTarget(0, val[0]);
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			LittleOCPDVFSSet(0, val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			LittleOCPEnable(0, val[0], val[1]);
		break;
	case 0:
		LittleOCPDisable(0);
		ocp_status[0].IntEnDis = 0;
		ocp_status[0].IRQ1 = 0;
		ocp_status[0].IRQ0 = 0;
		ocp_status[0].CapToLkg = 0;
		ocp_status[0].CapOCCGPct = 0;
		ocp_status[0].CaptureValid = 0;
		ocp_status[0].CapTotAct = 0;
		ocp_status[0].CapMAFAct = 0;
		ocp_status[0].CGAvgValid = 0;
		ocp_status[0].CGAvg = 0;
		ocp_status[0].AvgLkg = 0;
		ocp_status[0].TopRawLkg = 0;
		ocp_status[0].CPU0RawLkg = 0;
		ocp_status[0].CPU1RawLkg = 0;
		ocp_status[0].CPU2RawLkg = 0;
		ocp_status[0].CPU3RawLkg = 0;
		ocp_info("Cluster 0 Little OCP Disable\n");
		break;

	default:
		ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

static int ocp_cluster1_enable_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 1 OCP flag = %d\n", ocp_opt.ocp_cluster1_flag);
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_opt.ocp_cluster1_enable);
return 0;
}

static ssize_t ocp_cluster1_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[3];
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;


if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
		switch (function_id) {
		case 8:
			ocp_opt.ocp_cluster1_flag = 0;
			LittleOCPConfig(1, 300, 10000);
			LittleOCPSetTarget(1, 127000);
			LittleOCPDVFSSet(1, 338, 780);
			LittleOCPEnable(1, 1, 625);
			break;
		case 4:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				LittleOCPConfig(1, val[0], val[1]);
			break;
		case 3:
			if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
				LittleOCPSetTarget(1, val[0]);
			break;
		case 2:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				LittleOCPDVFSSet(1, val[0], val[1]);
			break;
		case 1:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				LittleOCPEnable(1, val[0], val[1]);
			break;
		case 0:
			LittleOCPDisable(1);
			ocp_status[1].IntEnDis = 0;
			ocp_status[1].IRQ1 = 0;
			ocp_status[1].IRQ0 = 0;
			ocp_status[1].CapToLkg = 0;
			ocp_status[1].CapOCCGPct = 0;
			ocp_status[1].CaptureValid = 0;
			ocp_status[1].CapTotAct = 0;
			ocp_status[1].CapMAFAct = 0;
			ocp_status[1].CGAvgValid = 0;
			ocp_status[1].CGAvg = 0;
			ocp_status[1].AvgLkg = 0;
			ocp_status[1].TopRawLkg = 0;
			ocp_status[1].CPU0RawLkg = 0;
			ocp_status[1].CPU1RawLkg = 0;
			ocp_status[1].CPU2RawLkg = 0;
			ocp_status[1].CPU3RawLkg = 0;
			ocp_info("Cluster 1 Little OCP Disable\n");
			break;

		default:
			ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

/* ocp_interrupt*/
static int ocp_cluster2_interrupt_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 2 OCP flag = %d\n", ocp_opt.ocp_cluster2_flag);
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_opt.ocp_cluster2_enable);
seq_puts(m, "Cluster 2 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[2].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[2].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[2].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[2].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[2].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[2].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[2].CapTotAct);
seq_printf(m, "CGAvgValid   = %d\n", ocp_status[2].CGAvgValid);
seq_printf(m, "CGAvg        = %llu %%\n", ocp_status[2].CGAvg);

return 0;
}

static ssize_t ocp_cluster2_interrupt_proc_write(struct file *file, const char __user *buffer,
size_t count, loff_t *pos)
{
int function_id, val[2], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
		switch (function_id) {
		case 0:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = BigOCPIntLimit(val[0], val[1]);
			break;
		case 1:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = BigOCPIntEnDis(val[0], val[1]);

			if (ret == 0) {
				ocp_status[2].IRQ1 = 0;
				ocp_status[2].IRQ0 = 0;
				ocp_status[2].IntEnDis = ((val[0] << 16) + val[1]);
			}
			break;
		case 2:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = BigOCPIntClr(val[0], val[1]);
			break;
		case 9:
			if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
				IRQ_Debug_on = val[0];

			ocp_info("Print IRQ_Debug_on = %d\n", IRQ_Debug_on);
			break;

		default:
			ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

static int ocp_cluster0_interrupt_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 0 OCP flag = %d\n", ocp_opt.ocp_cluster0_flag);
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_opt.ocp_cluster0_enable);
seq_puts(m, "Cluster 0 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[0].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[0].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[0].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[0].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[0].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[0].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[0].CapTotAct);

return 0;
}

static ssize_t ocp_cluster0_interrupt_proc_write(struct file *file, const char __user *buffer,
size_t count, loff_t *pos)
{
int function_id, val[2], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;


if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
	switch (function_id) {
	case 0:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntLimit(0, val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntEnDis(0, val[0], val[1]);

		if (ret == 0) {
			ocp_status[0].IRQ1 = 0;
			ocp_status[0].IRQ0 = 0;
			ocp_status[0].IntEnDis = ((val[0] << 16) + val[1]);
			}
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntClr(0, val[0], val[1]);
		break;
	case 9:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			IRQ_Debug_on = val[0];

		ocp_info("Print IRQ_Debug_on = %d\n", IRQ_Debug_on);
		break;
	default:
		ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

static int ocp_cluster1_interrupt_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 1 OCP flag = %d\n", ocp_opt.ocp_cluster1_flag);
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_opt.ocp_cluster1_enable);
seq_puts(m, "Cluster 1 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[1].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[1].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[1].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[1].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[1].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[1].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[1].CapTotAct);
return 0;
}

static ssize_t ocp_cluster1_interrupt_proc_write(struct file *file, const char __user *buffer,
size_t count, loff_t *pos)
{
int function_id, val[2], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;


if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
	switch (function_id) {
	case 0:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntLimit(1, val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntEnDis(1, val[0], val[1]);

		if (ret == 0) {
			ocp_status[1].IRQ1 = 0;
			ocp_status[1].IRQ0 = 0;
			ocp_status[1].IntEnDis = ((val[0] << 16) + val[1]);
		}
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPIntClr(1, val[0], val[1]);
		break;
	case 9:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			IRQ_Debug_on = val[0];

		ocp_info("Print IRQ_Debug_on = %d\n", IRQ_Debug_on);
		break;
	default:
		ocp_err("Invalid function code.\r\n");
	}
}

free_page((unsigned long)buf);
return count;
}

/* ocp_capture */

static int ocp_cluster2_capture_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 2 OCP flag = %d\n", ocp_opt.ocp_cluster2_flag);
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_opt.ocp_cluster2_enable);
seq_puts(m, "Cluster 2 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[2].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[2].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[2].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[2].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[2].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[2].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[2].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA\n", ocp_status[2].CapMAFAct);
seq_printf(m, "CGAvgValid   = %d\n", ocp_status[2].CGAvgValid);
seq_printf(m, "CGAvg        = %llu %%\n", ocp_status[2].CGAvg);
seq_printf(m, "TopRawLkg    = %d\n", ocp_status[2].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d\n", ocp_status[2].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d\n", ocp_status[2].CPU1RawLkg);

if (Reg_Debug_on) {
	int i;

	for (i = 0; i < 148; i += 4)
		seq_printf(m, "Addr: 0x%x = %x\n", (OCPAPBSTATUS00 + i), ocp_read(OCPAPBSTATUS00 + i));

	seq_printf(m, "Addr: 0x10206660 = %x\n", _ocp_read(OCPEFUS_BASE + 0x660));
	seq_printf(m, "Addr: 0x10206664 = %x\n", _ocp_read(OCPEFUS_BASE + 0x664));
	seq_printf(m, "Addr: 0x10206668 = %x\n", _ocp_read(OCPEFUS_BASE + 0x668));
	seq_printf(m, "Addr: 0x1020666C = %x\n", _ocp_read(OCPEFUS_BASE + 0x66C));
}

return 0;
}

static ssize_t ocp_cluster2_capture_proc_write(struct file *file, const char __user *buffer, size_t count,
loff_t *pos)
{
int EnDis, Edge, Count1, Trig, CGAvgSel;
int Leakage, Total, ClkPct;
unsigned int CGAvg, CapMAFAct;
int TopRawLkg, CPU0RawLkg, CPU1RawLkg;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) > 0) {
	switch (EnDis) {
	case 0:
		BigOCPCapture(0, 0, 0, 0);
		ocp_status[2].CapToLkg = 0;
		ocp_status[2].CapTotAct = 0;
		ocp_status[2].CapOCCGPct = 0;
		ocp_status[2].CaptureValid = 0;
		ocp_status[2].CapMAFAct = 0;
		ocp_status[2].CGAvgValid = 0;
		ocp_status[2].CGAvg = 0;
		ocp_status[2].AvgLkg = 0;
		ocp_status[2].TopRawLkg = 0;
		ocp_status[2].CPU0RawLkg = 0;
		ocp_status[2].CPU1RawLkg = 0;
		ocp_status[2].CPU2RawLkg = 0;
		ocp_status[2].CPU3RawLkg = 0;
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) == 4) {
			if (BigOCPCapture(1, Edge, Count1, Trig) != 0)
				ocp_err("BigOCPCapture enable fail!");
		}

		break;
	case 2:
		BigOCPCaptureStatus(&Leakage, &Total, &ClkPct);
		ocp_status[2].CapToLkg = Leakage;
		ocp_status[2].CapTotAct = Total;
		ocp_status[2].CapOCCGPct = ClkPct;
/*		ocp_status[2].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);

		BigOCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg);
		ocp_status[2].TopRawLkg = TopRawLkg;
		ocp_status[2].CPU0RawLkg = CPU0RawLkg;
		ocp_status[2].CPU1RawLkg = CPU1RawLkg;

		BigOCPMAFAct(&CapMAFAct);
		ocp_status[2].CapMAFAct = CapMAFAct;
*/
		break;
	case 3:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			ocp_info("Total Power = %d mA\n", BigOCPAvgPwrGet(Count1));

		break;
	case 4:
		BigOCPClkAvg(0, 0);
		ocp_status[2].CGAvgValid = 0;
		ocp_status[2].CGAvg = 0;
		break;
	case 5:
		if (sscanf(buf, "%d %d", &EnDis, &CGAvgSel) == 2) {
			if (BigOCPClkAvg(1, CGAvgSel) != 0)
				ocp_err("BigOCPClkAvg enable fail!");
		}
		break;
	case 6:
		BigOCPClkAvgStatus(&CGAvg);
		ocp_status[2].CGAvgValid = ocp_read_field(OCPAPBSTATUS04, 27:27);
		ocp_status[2].CGAvg = CGAvg;
		break;
	case 8:
		BigOCPClkAvg(1, 0);
		mdelay(1);
		BigOCPCapture(1, 1, 0, 15);
		BigOCPCaptureStatus(&Leakage, &Total, &ClkPct);
		ocp_status[2].CapToLkg = Leakage;
		ocp_status[2].CapTotAct = Total;
		ocp_status[2].CapOCCGPct = ClkPct;
		ocp_status[2].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);

		BigOCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg);
		ocp_status[2].TopRawLkg = TopRawLkg;
		ocp_status[2].CPU0RawLkg = CPU0RawLkg;
		ocp_status[2].CPU1RawLkg = CPU1RawLkg;

		BigOCPMAFAct(&CapMAFAct);
		ocp_status[2].CapMAFAct = CapMAFAct;

		BigOCPClkAvgStatus(&CGAvg);
		ocp_status[2].CGAvgValid = ocp_read_field(OCPAPBSTATUS04, 27:27);
		ocp_status[2].CGAvg = CGAvg;
		break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;
		break;
	default:
		ocp_err("Please Echo 1/0 to Enable/Disable OCP capture.\n");
	}
}
free_page((unsigned long)buf);
return count;
}

static int ocp_cluster0_capture_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 0 OCP flag = %d\n", ocp_opt.ocp_cluster0_flag);
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_opt.ocp_cluster0_enable);
seq_puts(m, "Cluster 0 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[0].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[0].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[0].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[0].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[0].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[0].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[0].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA\n", ocp_status[0].CapMAFAct);
seq_printf(m, "AvgPwrValid  = %d\n", ocp_status[0].CGAvgValid);
seq_printf(m, "AvgAct       = %llu mA\n", ocp_status[0].CGAvg);
seq_printf(m, "AvgLkg       = %llu mA\n", ocp_status[0].AvgLkg);
seq_printf(m, "TopRawLkg    = %d\n", ocp_status[0].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d\n", ocp_status[0].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d\n", ocp_status[0].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg   = %d\n", ocp_status[0].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg   = %d\n", ocp_status[0].CPU3RawLkg);

if (Reg_Debug_on) {
	int i;

	for (i = 0; i < 524 ; i += 4)
		seq_printf(m, "Addr: 0x%x = %x\n", (MP0_OCP_IRQSTATE + i), ocp_read(MP0_OCP_IRQSTATE + i));

		seq_printf(m, "Addr: 0x102217FC = %x\n", ocp_read(MP0_OCP_GENERAL_CTRL));
		seq_printf(m, "Addr: 0x10221250 = %x\n", ocp_read(MP0_OCP_DBG_ACT_L));
		seq_printf(m, "Addr: 0x10221254 = %x\n", ocp_read(MP0_OCP_DBG_ACT_H));
		seq_printf(m, "Addr: 0x10221260 = %x\n", ocp_read(MP0_OCP_DBG_LKG_L));
		seq_printf(m, "Addr: 0x10221264 = %x\n", ocp_read(MP0_OCP_DBG_LKG_H));
}
return 0;
}

static ssize_t ocp_cluster0_capture_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int EnDis, Edge, Count1, Trig;
int Leakage, Total, ClkPct, CapMAFAct;
unsigned long long AvgAct, AvgLkg;
int TopRawLkg, CPU0RawLkg, CPU1RawLkg, CPU2RawLkg, CPU3RawLkg;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) > 0) {
	switch (EnDis) {
	case 0:
		LittleLowerPowerOff(0, 1);
		LittleOCPCapture(0, 0, 0, 0, 0);
		ocp_status[0].CapToLkg = 0;
		ocp_status[0].CapTotAct = 0;
		ocp_status[0].CapOCCGPct = 0;
		ocp_status[0].CaptureValid = 0;
		ocp_status[0].CapMAFAct = 0;
		ocp_status[0].CGAvgValid = 0;
		ocp_status[0].CGAvg = 0;
		ocp_status[0].AvgLkg = 0;
		ocp_status[0].TopRawLkg = 0;
		ocp_status[0].CPU0RawLkg = 0;
		ocp_status[0].CPU1RawLkg = 0;
		ocp_status[0].CPU2RawLkg = 0;
		ocp_status[0].CPU3RawLkg = 0;
		LittleLowerPowerOn(0, 0);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) == 4) {
			LittleLowerPowerOff(0, 1);
			if (LittleOCPCapture(0, 1, Edge, Count1, Trig) != 0)
				ocp_err("ocp_cluster0_capture enable fail!");
		}
		break;
	case 2:
		LittleOCPCaptureGet(0, &Leakage, &Total, &ClkPct);
		ocp_status[0].CapToLkg = Leakage;
		ocp_status[0].CapTotAct = Total;
		ocp_status[0].CapOCCGPct = ClkPct;
		ocp_status[0].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);

		CL0OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[0].TopRawLkg = TopRawLkg;
		ocp_status[0].CPU0RawLkg = CPU0RawLkg;
		ocp_status[0].CPU1RawLkg = CPU1RawLkg;
		ocp_status[0].CPU2RawLkg = CPU2RawLkg;
		ocp_status[0].CPU3RawLkg = CPU3RawLkg;

		LittleOCPMAFAct(0, &CapMAFAct);
		ocp_status[0].CapMAFAct = CapMAFAct;

		LittleLowerPowerOn(0, 0);
		break;
	case 3:
			if (sscanf(buf, "%d %d %d", &EnDis, &Edge, &Count1) == 3) {
				do_ocp_test_on = 0;
				if (LittleOCPAvgPwr(0, Edge, Count1) != 0)
					ocp_err("LittleOCPAvgPwr enable fail!");
			}
			break;
	case 4:
			do_ocp_test_on = 0;
			ocp_status[0].CGAvgValid = ocp_read_field(MP0_OCP_DBG_STAT, 31:31);
			ocp_status[0].CGAvg = LittleOCPAvgPwrGet(0);
			ocp_status[0].AvgLkg = 0;
			break;
	case 8: /* capture all*/
		LittleLowerPowerOff(0, 1);
		LittleOCPDVFSSet(0, mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000,
					mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100);
		LittleOCPAvgPwr_HQA(0, 1, mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL));
		mdelay(1);
		LittleOCPCapture(0, 1, 1, 0, 15);
		LittleOCPCaptureGet(0, &Leakage, &Total, &ClkPct);
		ocp_status[0].CapToLkg = Leakage;
		ocp_status[0].CapTotAct = Total;
		ocp_status[0].CapOCCGPct = ClkPct;
		ocp_status[0].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);

		CL0OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[0].TopRawLkg = TopRawLkg;
		ocp_status[0].CPU0RawLkg = CPU0RawLkg;
		ocp_status[0].CPU1RawLkg = CPU1RawLkg;
		ocp_status[0].CPU2RawLkg = CPU2RawLkg;
		ocp_status[0].CPU3RawLkg = CPU3RawLkg;

		LittleOCPMAFAct(0, &CapMAFAct);
		ocp_status[0].CapMAFAct = CapMAFAct;

		LittleOCPAvgPwrGet_HQA(0, &AvgLkg, &AvgAct);
		ocp_status[0].CGAvgValid = ocp_read_field(MP0_OCP_DBG_STAT, 31:31);
		ocp_status[0].CGAvg = AvgAct;
		ocp_status[0].AvgLkg = AvgLkg;

		LittleLowerPowerOn(0, 0);
		break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;
		break;
	default:
		ocp_err("Please Echo 1/0 to Enable/Disable OCP capture.\n");
	}
}
free_page((unsigned long)buf);
return count;
}

static int ocp_cluster1_capture_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Cluster 1 OCP flag = %d\n", ocp_opt.ocp_cluster1_flag);
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_opt.ocp_cluster1_enable);
seq_puts(m, "Cluster 1 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[1].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[1].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[1].IRQ0);
seq_printf(m, "CapToLkg     = %d mA\n", ocp_status[1].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[1].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[1].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA\n", ocp_status[1].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA\n", ocp_status[1].CapMAFAct);
seq_printf(m, "AvgPwrValid  = %d\n", ocp_status[1].CGAvgValid);
seq_printf(m, "AvgAct       = %llu mA\n", ocp_status[1].CGAvg);
seq_printf(m, "AvgLkg       = %llu mA\n", ocp_status[1].AvgLkg);
seq_printf(m, "TopRawLkg    = %d\n", ocp_status[1].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d\n", ocp_status[1].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d\n", ocp_status[1].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg   = %d\n", ocp_status[1].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg   = %d\n", ocp_status[1].CPU3RawLkg);

if (Reg_Debug_on) {
	int i;

	for (i = 0; i < 524; i += 4)
		seq_printf(m, "Addr: 0x%x = %x\n", (MP1_OCP_IRQSTATE + i), ocp_read(MP1_OCP_IRQSTATE + i));

		seq_printf(m, "Addr: 0x102237FC = %x\n", ocp_read(MP1_OCP_GENERAL_CTRL));
		seq_printf(m, "Addr: 0x10223250 = %x\n", ocp_read(MP1_OCP_DBG_ACT_L));
		seq_printf(m, "Addr: 0x10223254 = %x\n", ocp_read(MP1_OCP_DBG_ACT_H));
		seq_printf(m, "Addr: 0x10223260 = %x\n", ocp_read(MP1_OCP_DBG_LKG_L));
		seq_printf(m, "Addr: 0x10223264 = %x\n", ocp_read(MP1_OCP_DBG_LKG_H));
}

return 0;
}

static ssize_t ocp_cluster1_capture_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int EnDis, Edge, Count1, Trig;
int Leakage, Total, ClkPct, CapMAFAct;
unsigned long long AvgAct, AvgLkg;
int TopRawLkg, CPU0RawLkg, CPU1RawLkg, CPU2RawLkg, CPU3RawLkg;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;


if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) > 0) {
	switch (EnDis) {
	case 0:
		LittleLowerPowerOff(1, 1);
		LittleOCPCapture(1, 0, 0, 0, 0);
		ocp_status[1].CapToLkg = 0;
		ocp_status[1].CapTotAct = 0;
		ocp_status[1].CapOCCGPct = 0;
		ocp_status[1].CaptureValid = 0;
		ocp_status[1].CapMAFAct = 0;
		ocp_status[1].CGAvgValid = 0;
		ocp_status[1].CGAvg = 0;
		ocp_status[1].AvgLkg = 0;
		ocp_status[1].TopRawLkg = 0;
		ocp_status[1].CPU0RawLkg = 0;
		ocp_status[1].CPU1RawLkg = 0;
		ocp_status[1].CPU2RawLkg = 0;
		ocp_status[1].CPU3RawLkg = 0;
		LittleLowerPowerOn(1, 0);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) == 4) {
			LittleLowerPowerOff(1, 1);
			if (LittleOCPCapture(1, 1, Edge, Count1, Trig) != 0)
				ocp_err("ocp_cluster1_capture enable fail!");
		}
		break;
	case 2:
		LittleOCPCaptureGet(1, &Leakage, &Total, &ClkPct);
		ocp_status[1].CapToLkg = Leakage;
		ocp_status[1].CapTotAct = Total;
		ocp_status[1].CapOCCGPct = ClkPct;
		ocp_status[1].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);

		CL1OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[1].TopRawLkg = TopRawLkg;
		ocp_status[1].CPU0RawLkg = CPU0RawLkg;
		ocp_status[1].CPU1RawLkg = CPU1RawLkg;
		ocp_status[1].CPU2RawLkg = CPU2RawLkg;
		ocp_status[1].CPU3RawLkg = CPU3RawLkg;

		LittleOCPMAFAct(1, &CapMAFAct);
		ocp_status[1].CapMAFAct = CapMAFAct;
		LittleLowerPowerOn(1, 0);
		break;
	case 3:
			if (sscanf(buf, "%d %d %d", &EnDis, &Edge, &Count1) == 3) {
				do_ocp_test_on = 0;
				if (LittleOCPAvgPwr(1, Edge, Count1) != 0)
					ocp_err("LittleOCPAvgPwr enable fail!");
			}
		break;
	case 4:
			do_ocp_test_on = 0;
			ocp_status[1].CGAvgValid = ocp_read_field(MP1_OCP_DBG_STAT, 31:31);
			ocp_status[1].CGAvg = LittleOCPAvgPwrGet(1);
			ocp_status[1].AvgLkg = 0;
		break;
	case 8: /* capture all*/
		LittleLowerPowerOff(1, 1);
		LittleOCPDVFSSet(1, mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000,
					mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100);
		LittleOCPAvgPwr_HQA(1, 1, mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L));
		mdelay(1);
		LittleOCPCapture(1, 1, 1, 0, 15);
		LittleOCPCaptureGet(1, &Leakage, &Total, &ClkPct);
		ocp_status[1].CapToLkg = Leakage;
		ocp_status[1].CapTotAct = Total;
		ocp_status[1].CapOCCGPct = ClkPct;
		ocp_status[1].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);

		CL1OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[1].TopRawLkg = TopRawLkg;
		ocp_status[1].CPU0RawLkg = CPU0RawLkg;
		ocp_status[1].CPU1RawLkg = CPU1RawLkg;
		ocp_status[1].CPU2RawLkg = CPU2RawLkg;
		ocp_status[1].CPU3RawLkg = CPU3RawLkg;

		LittleOCPMAFAct(1, &CapMAFAct);
		ocp_status[1].CapMAFAct = CapMAFAct;

		LittleOCPAvgPwrGet_HQA(1, &AvgLkg, &AvgAct);
		ocp_status[1].CGAvgValid = ocp_read_field(MP1_OCP_DBG_STAT, 31:31);
		ocp_status[1].CGAvg = AvgAct;
		ocp_status[1].AvgLkg = AvgLkg;

		LittleLowerPowerOn(1, 0);
		break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;
		break;
	default:
		ocp_err("Please Echo 1/0 to Enable/Disable OCP capture.\n");
	}
}
free_page((unsigned long)buf);
return count;
}

/* dreq_function*/
static int dreq_function_test_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "Big DREQ HW Enable = %d\n", big_dreq_enable);
seq_printf(m, "       Big DREQ on = %d\n", BigDREQGet());
seq_printf(m, "    Little DREQ on = %x\n", LittleDREQGet());

if (Reg_Debug_on) {
	seq_printf(m, "BIG_SRAMDREQ:    Addr: 0x%x = %x\n", BIG_SRAMDREQ, ocp_read(BIG_SRAMDREQ));
	seq_printf(m, "LITTLE_SRAMDREQ: Addr: 0x%x = %x\n", LITTLE_SRAMDREQ, _ocp_read(OCPPROB_BASE));
	seq_printf(m, "BIG_SRAMLDO:     Addr: 0x%x = %x\n", BIG_SRAMLDO, ocp_read(BIG_SRAMLDO));
}
return 0;
}

static ssize_t dreq_function_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int ret = 0, function_id, val[3];
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
	switch (function_id) {
	case 0:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			ret = BigDREQHWEn(val[0], val[1]);
			if (ret == 0)
				big_dreq_enable = 1;
		}
		break;
	case 1:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			ret = BigDREQSWEn(val[0]);

			if (val[0] == 1 && ret == 0)
				big_dreq_enable = 1;
			else if (val[0] == 0 && ret == 0)
				big_dreq_enable = 0;
		}
		break;
	case 2:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			LittleDREQSWEn(val[0]);
		break;
	case 3:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			ret = BigSRAMLDOEnable(val[0]);
			if (ret == 0)
				ocp_info("SRAM LDO volte = %dmv, enable success.\n", val[0]);
		}
		break;
	case 9:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			Reg_Debug_on = val[0];

		break;

	default:
		ocp_err("DREQ Error input parameter or function code.\r\n");
	}
}
free_page((unsigned long)buf);
return count;
}



/* dvt_test*/
static int dvt_test_proc_show(struct seq_file *m, void *v)
{

if (dvt_test_on == 130 || dvt_test_on == 230)
	seq_printf(m, "CPU0RawLkg = %d\n", ocp_status[2].CPU0RawLkg);
else if (dvt_test_on == 131 || dvt_test_on == 231)
	seq_printf(m, "CPU1RawLkg = %d\n", ocp_status[2].CPU1RawLkg);
else if (dvt_test_on == 132 || dvt_test_on == 232)
	seq_printf(m, "TopRawLkg  = %d\n", ocp_status[2].TopRawLkg);
else if (dvt_test_on == 110 || dvt_test_on == 210)
	seq_printf(m, "CPU0RawLkg = %d\n", ocp_status[0].CPU0RawLkg);
else if (dvt_test_on == 111 || dvt_test_on == 211)
	seq_printf(m, "CPU1RawLkg = %d\n", ocp_status[0].CPU1RawLkg);
else if (dvt_test_on == 112 || dvt_test_on == 212)
	seq_printf(m, "CPU2RawLkg = %d\n", ocp_status[0].CPU2RawLkg);
else if (dvt_test_on == 113 || dvt_test_on == 213)
	seq_printf(m, "CPU3RawLkg = %d\n", ocp_status[0].CPU3RawLkg);
else if (dvt_test_on == 114 || dvt_test_on == 214)
	seq_printf(m, "TopRawLkg  = %d\n", ocp_status[0].TopRawLkg);
else if (dvt_test_on == 120 || dvt_test_on == 220)
	seq_printf(m, "CPU0RawLkg = %d\n", ocp_status[1].CPU0RawLkg);
else if (dvt_test_on == 121 || dvt_test_on == 221)
	seq_printf(m, "CPU1RawLkg = %d\n", ocp_status[1].CPU1RawLkg);
else if (dvt_test_on == 122 || dvt_test_on == 222)
	seq_printf(m, "CPU2RawLkg = %d\n", ocp_status[1].CPU2RawLkg);
else if (dvt_test_on == 123 || dvt_test_on == 223)
	seq_printf(m, "CPU3RawLkg = %d\n", ocp_status[1].CPU3RawLkg);
else if (dvt_test_on == 124 || dvt_test_on == 224)
	seq_printf(m, "TopRawLkg  = %d\n", ocp_status[1].TopRawLkg);

if (dvt_test_on == 1) {
	if (dvt_test_set < 0x3000)
		seq_printf(m, "reg: 0x10001000 + 0x%x [%d:%d] = %x\n", dvt_test_set, dvt_test_msb, dvt_test_lsb,
			_ocp_read_field(OCPPROB_BASE + dvt_test_set, dvt_test_msb:dvt_test_lsb));
	else
		seq_printf(m, "reg:0x%x [%d:%d] = %x\n", dvt_test_set, dvt_test_msb, dvt_test_lsb,
			ocp_read_field(dvt_test_set, dvt_test_msb:dvt_test_lsb));
}
return 0;
}


static ssize_t dvt_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int i = 0, function_id, val[4];
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) > 0) {
	switch (function_id) {
	case 1:
		/* LkgMon TM1 */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			if (val[0] == 30) { /* Big LkgMon TM1 CPU0 */
				dvt_test_on = 130;
			if (val[1] == 1) {
				BigOCPConfig(300, 10000);
				BigOCPSetTarget(1, 127000);

				ocp_write_field(0x10222584, 12:12, 0x1);
				ocp_write_field(0x10222584, 18:18, 0x1);

				BigOCPEnable(1, 0, 625, 0);
			}

			ocp_write_field(0x10222584, 16:16, 0x1);
			ocp_write_field(0x10222584, 16:16, 0x0);
			ocp_write_field(0x10222584, 16:16, 0x1);
			ocp_write_field(0x10222584, 16:16, 0x0);
		} else if (val[0] == 31) {
			/* Big LkgMon TM1 CPU1 */
			dvt_test_on = 131;
			if (val[1] == 1) {

				BigOCPConfig(300, 10000);
				BigOCPSetTarget(1, 127000);

				ocp_write_field(0x10222584, 13:13, 0x1);
				ocp_write_field(0x10222584, 19:19, 0x1);

				BigOCPEnable(1, 0, 625, 0);
			}

			ocp_write_field(0x10222584, 17:17, 0x1);
			ocp_write_field(0x10222584, 17:17, 0x0);
			ocp_write_field(0x10222584, 17:17, 0x1);
			ocp_write_field(0x10222584, 17:17, 0x0);
		} else if (val[0] == 32) {
			/* Big LkgMon TM1 TOP */
			dvt_test_on = 132;
			if (val[1] == 1) {
				BigOCPConfig(300, 10000);
				BigOCPSetTarget(1, 127000);

				ocp_write_field(0x10222584, 1:1, 0x1);
				ocp_write_field(0x10222584, 4:4, 0x1);

				BigOCPEnable(1, 0, 625, 0);
			}

			ocp_write_field(0x10222584, 3:3, 0x1);
			ocp_write_field(0x10222584, 3:3, 0x0);
			ocp_write_field(0x10222584, 3:3, 0x1);
			ocp_write_field(0x10222584, 3:3, 0x0);
		} else if (val[0] == 10) {
			/* LL LkgMon TM1 CPU0 */
			dvt_test_on = 110;
			if (val[1] == 1) {
				LittleOCPConfig(0, 300, 10000);
				LittleOCPSetTarget(0, 127000);

				ocp_write_field(0x10221164, 4:4, 0x1);
				ocp_write_field(0x10221164, 16:16, 0x1);

				LittleOCPEnable(0, 0, 625);
			}

			ocp_write_field(0x10221164, 12:12, 0x1);
			ocp_write_field(0x10221164, 12:12, 0x0);
			ocp_write_field(0x10221164, 12:12, 0x1);
			ocp_write_field(0x10221164, 12:12, 0x0);
		} else if (val[0] == 11) {
			/* LL LkgMon TM1 CPU1 */
			dvt_test_on = 111;
			if (val[1] == 1) {
				LittleOCPConfig(0, 300, 10000);
				LittleOCPSetTarget(0, 127000);

				ocp_write_field(0x10221164, 5:5, 0x1);
				ocp_write_field(0x10221164, 17:17, 0x1);

				LittleOCPEnable(0, 0, 625);
			}

			ocp_write_field(0x10221164, 13:13, 0x1);
			ocp_write_field(0x10221164, 13:13, 0x0);
			ocp_write_field(0x10221164, 13:13, 0x1);
			ocp_write_field(0x10221164, 13:13, 0x0);
		} else if (val[0] == 12) {
			/* LL LkgMon TM1 CPU2 */
			dvt_test_on = 112;
			if (val[1] == 1) {
				LittleOCPConfig(0, 300, 10000);
				LittleOCPSetTarget(0, 127000);

				ocp_write_field(0x10221164, 6:6, 0x1);
				ocp_write_field(0x10221164, 18:18, 0x1);

				LittleOCPEnable(0, 0, 625);
			}

			ocp_write_field(0x10221164, 14:14, 0x1);
			ocp_write_field(0x10221164, 14:14, 0x0);
			ocp_write_field(0x10221164, 14:14, 0x1);
			ocp_write_field(0x10221164, 14:14, 0x0);
		} else if (val[0] == 13) {
			/* LL LkgMon TM1 CPU3 */
			dvt_test_on = 113;
			if (val[1] == 1) {
				LittleOCPConfig(0, 300, 10000);
				LittleOCPSetTarget(0, 127000);

				ocp_write_field(0x10221164, 7:7, 0x1);
				ocp_write_field(0x10221164, 19:19, 0x1);

				LittleOCPEnable(0, 0, 625);
			}

			ocp_write_field(0x10221164, 15:15, 0x1);
			ocp_write_field(0x10221164, 15:15, 0x0);
			ocp_write_field(0x10221164, 15:15, 0x1);
			ocp_write_field(0x10221164, 15:15, 0x0);
		} else if (val[0] == 14) {
			/* LL LkgMon TM1 TOP */
			dvt_test_on = 114;
			if (val[1] == 1) {
				LittleOCPConfig(0, 300, 10000);
				LittleOCPSetTarget(0, 127000);

				ocp_write_field(0x10221160, 1:1, 0x1);
				ocp_write_field(0x10221160, 4:4, 0x1);

				LittleOCPEnable(0, 0, 625);
			}
			ocp_write_field(0x10221160, 3:3, 0x1);
			ocp_write_field(0x10221160, 3:3, 0x0);
			ocp_write_field(0x10221160, 3:3, 0x1);
			ocp_write_field(0x10221160, 3:3, 0x0);
		} else if (val[0] == 20) {
			/* L LkgMon TM1 CPU0 */
			dvt_test_on = 120;
			if (val[1] == 1) {
				LittleOCPConfig(1, 300, 10000);
				LittleOCPSetTarget(1, 127000);

				ocp_write_field(0x10223164, 4:4, 0x1);
				ocp_write_field(0x10223164, 16:16, 0x1);

				LittleOCPEnable(1, 0, 625);
			}

			ocp_write_field(0x10223164, 12:12, 0x1);
			ocp_write_field(0x10223164, 12:12, 0x0);
			ocp_write_field(0x10223164, 12:12, 0x1);
			ocp_write_field(0x10223164, 12:12, 0x0);
		} else if (val[0] == 21) {
			/* L LkgMon TM1 CPU1 */
			dvt_test_on = 121;
			if (val[1] == 1) {
				LittleOCPConfig(1, 300, 10000);
				LittleOCPSetTarget(1, 127000);

				ocp_write_field(0x10223164, 5:5, 0x1);
				ocp_write_field(0x10223164, 17:17, 0x1);

				LittleOCPEnable(1, 0, 625);
			}

			ocp_write_field(0x10223164, 13:13, 0x1);
			ocp_write_field(0x10223164, 13:13, 0x0);
			ocp_write_field(0x10223164, 13:13, 0x1);
			ocp_write_field(0x10223164, 13:13, 0x0);
		} else if (val[0] == 22) {
			/* L LkgMon TM1 CPU2 */
			dvt_test_on = 122;
			if (val[1] == 1) {
				LittleOCPConfig(1, 300, 10000);
				LittleOCPSetTarget(1, 127000);

				ocp_write_field(0x10223164, 6:6, 0x1);
				ocp_write_field(0x10223164, 18:18, 0x1);

				LittleOCPEnable(1, 0, 625);
			}

			ocp_write_field(0x10223164, 14:14, 0x1);
			ocp_write_field(0x10223164, 14:14, 0x0);
			ocp_write_field(0x10223164, 14:14, 0x1);
			ocp_write_field(0x10223164, 14:14, 0x0);
		} else if (val[0] == 23) {
			/* L LkgMon TM1 CPU3 */
			dvt_test_on = 123;
			if (val[1] == 1) {
				LittleOCPConfig(1, 300, 10000);
				LittleOCPSetTarget(1, 127000);

				ocp_write_field(0x10223164, 7:7, 0x1);
				ocp_write_field(0x10223164, 19:19, 0x1);

				LittleOCPEnable(1, 0, 625);
			}

			ocp_write_field(0x10223164, 15:15, 0x1);
			ocp_write_field(0x10223164, 15:15, 0x0);
			ocp_write_field(0x10223164, 15:15, 0x1);
			ocp_write_field(0x10223164, 15:15, 0x0);
		} else if (val[0] == 24) {
			/* LL LkgMon TM1 TOP */
			dvt_test_on = 124;
			if (val[1] == 1) {
				LittleOCPConfig(1, 300, 10000);
				LittleOCPSetTarget(1, 127000);

				ocp_write_field(0x10223160, 1:1, 0x1);
				ocp_write_field(0x10223160, 4:4, 0x1);

				LittleOCPEnable(1, 0, 625);
			}

			ocp_write_field(0x10223160, 3:3, 0x1);
			ocp_write_field(0x10223160, 3:3, 0x0);
			ocp_write_field(0x10223160, 3:3, 0x1);
			ocp_write_field(0x10223160, 3:3, 0x0);
			}
		}
		break;
	case 2:
		/* LkgMon TM2 */
		if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5) {
			if (val[0] == 30) {
				/* Big LkgMon TM2 CPU0 */
				dvt_test_on = 230;
				if (val[1] == 1) {
					/* Cpu0LkgTM2 = 1 */
					ocp_write_field(0x10222584, 14:14, 0x1);
					/* Cpu0LkgTrim = 0	*/
					ocp_write_field(0x10222530, 7:4, 0x0);
					/* Cpu0LkgInt = 0	*/
					ocp_write_field(0x10222584, 10:10, 0x0);
					/* Cpu0LkgInt = 1 */
					ocp_write_field(0x10222584, 10:10, 0x1);
				}
				if (val[2] == 1) {
					/* Cpu0LkgTrim = 9 */
					ocp_write_field(0x10222530, 7:4, 0x9);
				}
				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						/* Cpu0LkgTestClk =1 */
						ocp_write_field(0x10222584, 16:16, 0x1);
						/* Cpu0LkgTestClk =0 */
						ocp_write_field(0x10222584, 16:16, 0x0);
					}
				}

			} else if (val[0] == 31) {
				/* Big LkgMon TM2 CPU1 */
				dvt_test_on = 231;
				if (val[1] == 1) {
					ocp_write_field(0x10222584, 15:15, 0x1);
					ocp_write_field(0x10222530, 11:8, 0x0);
					ocp_write_field(0x10222584, 11:11, 0x0);
					ocp_write_field(0x10222584, 11:11, 0x1);
				}
				if (val[2] == 1)
					ocp_write_field(0x10222530, 11:8, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10222584, 17:17, 0x1);
						ocp_write_field(0x10222584, 17:17, 0x0);
					}
				}
			} else if (val[0] == 32) {
				/* Big LkgMon TM2 TOP */
				dvt_test_on = 232;
				if (val[1] == 1) {
					ocp_write_field(0x10222584, 2:2, 0x1);
					ocp_write_field(0x10222530, 3:0, 0x0);
					ocp_write_field(0x10222584, 0:0, 0x0);
					ocp_write_field(0x10222584, 0:0, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10222530, 3:0, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10222584, 3:3, 0x1);
						ocp_write_field(0x10222584, 3:3, 0x0);
					}
				}
			} else if (val[0] == 10) {
				/* LL LkgMon TM2 CPU0 */
				dvt_test_on = 210;
				if (val[1] == 1) {
					ocp_write_field(0x10221164, 8:8, 0x1);
					ocp_write_field(0x10221058, 7:4, 0x0);
					ocp_write_field(0x10221164, 0:0, 0x0);
					ocp_write_field(0x10221164, 0:0, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10221058, 7:4, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10221164, 12:12, 0x1);
						ocp_write_field(0x10221164, 12:12, 0x0);
					}
				}
			} else if (val[0] == 11) {
				/* LL LkgMon TM2 CPU1 */
				dvt_test_on = 211;
				if (val[1] == 1) {
					ocp_write_field(0x10221164, 9:9, 0x1);
					ocp_write_field(0x10221058, 11:8, 0x0);
					ocp_write_field(0x10221164, 1:1, 0x0);
					ocp_write_field(0x10221164, 1:1, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10221058, 11:8, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10221164, 13:13, 0x1);
						ocp_write_field(0x10221164, 13:13, 0x0);
					}
				}
			} else if (val[0] == 12) {
				/* LL LkgMon TM2 CPU2 */
				dvt_test_on = 212;
				if (val[1] == 1) {
					ocp_write_field(0x10221164, 10:10, 0x1);
					ocp_write_field(0x10221058, 15:12, 0x0);
					ocp_write_field(0x10221164, 2:2, 0x0);
					ocp_write_field(0x10221164, 2:2, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10221058, 15:12, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10221164, 14:14, 0x1);
						ocp_write_field(0x10221164, 14:14, 0x0);
					}
				}
			} else if (val[0] == 13) {
				/* LL LkgMon TM2 CPU3 */
				dvt_test_on = 213;
				if (val[1] == 1) {
					ocp_write_field(0x10221164, 11:11, 0x1);
					ocp_write_field(0x10221058, 19:16, 0x0);
					ocp_write_field(0x10221164, 3:3, 0x0);
					ocp_write_field(0x10221164, 3:3, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10221058, 19:16, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10221164, 15:15, 0x1);
						ocp_write_field(0x10221164, 15:15, 0x0);
					}
				}
			} else if (val[0] == 14) {
				/* LL LkgMon TM2 TOP */
				dvt_test_on = 214;
				if (val[1] == 1) {
					ocp_write_field(0x10221160, 2:2, 0x1);
					ocp_write_field(0x10221058, 3:0, 0x0);
					ocp_write_field(0x10221160, 0:0, 0x0);
					ocp_write_field(0x10221160, 0:0, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10221058, 3:0, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10221160, 3:3, 0x1);
						ocp_write_field(0x10221160, 3:3, 0x0);
					}
				}
			} else if (val[0] == 20) {
				/* L LkgMon TM2 CPU0 */
				dvt_test_on = 220;
				if (val[1] == 1) {
					ocp_write_field(0x10223164, 8:8, 0x1);
					ocp_write_field(0x10223058, 7:4, 0x0);
					ocp_write_field(0x10223164, 0:0, 0x0);
					ocp_write_field(0x10223164, 0:0, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10223058, 7:4, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10223164, 12:12, 0x1);
						ocp_write_field(0x10223164, 12:12, 0x0);
					}
				}
			} else if (val[0] == 21) {
				/* L LkgMon TM2 CPU1 */
				dvt_test_on = 221;
				if (val[1] == 1) {
					ocp_write_field(0x10223164, 9:9, 0x1);
					ocp_write_field(0x10223058, 11:8, 0x0);
					ocp_write_field(0x10223164, 1:1, 0x0);
					ocp_write_field(0x10223164, 1:1, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10223058, 11:8, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10223164, 13:13, 0x1);
						ocp_write_field(0x10223164, 13:13, 0x0);
					}
				}
			} else if (val[0] == 22) {
				/* L LkgMon TM2 CPU2 */
				dvt_test_on = 222;
				if (val[1] == 1) {
					ocp_write_field(0x10223164, 10:10, 0x1);
					ocp_write_field(0x10223058, 15:12, 0x0);
					ocp_write_field(0x10223164, 2:2, 0x0);
					ocp_write_field(0x10223164, 2:2, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10223058, 15:12, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10223164, 14:14, 0x1);
						ocp_write_field(0x10223164, 14:14, 0x0);
					}
				}
			} else if (val[0] == 23) {
				/* L LkgMon TM2 CPU3 */
				dvt_test_on = 223;
				if (val[1] == 1) {
					ocp_write_field(0x10223164, 11:11, 0x1);
					ocp_write_field(0x10223058, 19:16, 0x0);
					ocp_write_field(0x10223164, 3:3, 0x0);
					ocp_write_field(0x10223164, 3:3, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10223058, 19:16, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10223164, 15:15, 0x1);
						ocp_write_field(0x10223164, 15:15, 0x0);
					}
				}
			} else if (val[0] == 24) {
				/* L LkgMon TM2 TOP */
				dvt_test_on = 224;
				if (val[1] == 1) {
					ocp_write_field(0x10223160, 2:2, 0x1);
					ocp_write_field(0x10223058, 3:0, 0x0);
					ocp_write_field(0x10223160, 0:0, 0x0);
					ocp_write_field(0x10223160, 0:0, 0x1);
				}

				if (val[2] == 1)
					ocp_write_field(0x10223058, 3:0, 0x9);

				if (val[3] == 1) {
					for (i = 0; i < 257; i++) {
						ocp_write_field(0x10223160, 3:3, 0x1);
						ocp_write_field(0x10223160, 3:3, 0x0);
					}
				}
			}

}

			break;
	case 3:
		/* LkgMon IREF */
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			if (val[0] == 30) {
				/* Big LkgMon IREF CPU0	*/
				ocp_write_field(0x10222700, 11:6, 0x0);
				ocp_write_field(0x10222584, 29:20, 0x0);
				ocp_write_field(0x10222584, 9:5, 0x0);
				ocp_write_field(0x102222B0, 31:24, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1890, ~(1<<6));
				ocp_write_field(0x10222584, 20:20, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10222530, 7:4, i);
					mdelay(10000);
				}

				ocp_write_field(0x10222584, 20:20, 0x0);

			} else if (val[0] == 31) {
				/* Big LkgMon IREF CPU1	 */
				ocp_write_field(0x10222700, 11:6, 0x0);
				ocp_write_field(0x10222584, 29:20, 0x0);
				ocp_write_field(0x10222584, 9:5, 0x0);
				ocp_write_field(0x102222B0, 31:24, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1890, ~(1<<6));
				ocp_write_field(0x10222584, 25:25, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10222530, 11:8, i);
					mdelay(10000);
				}

				ocp_write_field(0x10222584, 25:25, 0x0);

			} else if (val[0] == 32) {
				/* Big LkgMon IREF TOP	 */
				ocp_write_field(0x10222700, 11:6, 0x0);
				ocp_write_field(0x10222584, 29:20, 0x0);
				ocp_write_field(0x10222584, 9:5, 0x0);
				ocp_write_field(0x102222B0, 31:24, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1890, ~(1<<6));
				ocp_write_field(0x10222584, 5:5, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10222530, 3:0, i);
					mdelay(10000);
				}
				ocp_write_field(0x10222584, 5:5, 0x0);

			} else if (val[0] == 10) {
				/* LL LkgMon IREF CPU0 */
				ocp_write_field(0x102211FC, 0:0, 0x0);
				ocp_write_field(0x102217FC, 2:0, 0x6);
				ocp_write_field(0x10221058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10221040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10221160, 0:0, 0x1);
				ocp_write_field(0x10221164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10221C08, 2:0, 0x0);
				ocp_write_field(0x10221C08, 10:8, 0x0);
				ocp_write_field(0x10221C08, 18:16, 0x0);
				ocp_write_field(0x10221C08, 26:24, 0x0);

				/* LkgMon AMUXSEL */
				ocp_write_field(0x10221168, 19:0, 0x0);
				ocp_write_field(0x10221160, 9:5 , 0x0);

				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10221168, 0:0, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10221058, 7:4, i);
					mdelay(10000);
				}

				ocp_write_field(0x10221168, 0:0, 0x0);

			} else if (val[0] == 11) {
				/* LL LkgMon IREF CPU1 */
				ocp_write_field(0x102211FC, 0:0, 0x0);
				ocp_write_field(0x102217FC, 2:0, 0x6);
				ocp_write_field(0x10221058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10221040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10221160, 0:0, 0x1);
				ocp_write_field(0x10221164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10221C08, 2:0, 0x0);
				ocp_write_field(0x10221C08, 10:8, 0x0);
				ocp_write_field(0x10221C08, 18:16, 0x0);
				ocp_write_field(0x10221C08, 26:24, 0x0);

				/* LkgMon AMUXSEL  */
				ocp_write_field(0x10221168, 19:0, 0x0);
				ocp_write_field(0x10221160, 9:5, 0x0);

				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10221168, 5:5, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10221058, 11:8, i);
					mdelay(10000);
				}

				ocp_write_field(0x10221168, 5:5, 0x0);

			} else if (val[0] == 12) {
				/* LL LkgMon IREF CPU2 */
				ocp_write_field(0x102211FC, 0:0, 0x0);
				ocp_write_field(0x102217FC, 2:0, 0x6);
				ocp_write_field(0x10221058, 19:0, 0x77777);

				/* Enable LkgMonEn  */
				ocp_write(0x10221040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10221160, 0:0, 0x1);
				ocp_write_field(0x10221164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10221C08, 2:0, 0x0);
				ocp_write_field(0x10221C08, 10:8, 0x0);
				ocp_write_field(0x10221C08, 18:16, 0x0);
				ocp_write_field(0x10221C08, 26:24, 0x0);

				/* LkgMon AMUXSEL  */
				ocp_write_field(0x10221168, 19:0, 0x0);
				ocp_write_field(0x10221160, 9:5, 0x0);

				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10221168, 10:10, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10221058, 15:12, i);
					mdelay(10000);
				}

				ocp_write_field(0x10221168, 10:10, 0x0);

			} else if (val[0] == 13) {
				/* LL LkgMon IREF CPU3 */
				ocp_write_field(0x102211FC, 0:0, 0x0);
				ocp_write_field(0x102217FC, 2:0, 0x6);
				ocp_write_field(0x10221058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10221040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10221160, 0:0, 0x1);
				ocp_write_field(0x10221164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10221C08, 2:0, 0x0);
				ocp_write_field(0x10221C08, 10:8, 0x0);
				ocp_write_field(0x10221C08, 18:16, 0x0);
				ocp_write_field(0x10221C08, 26:24, 0x0);

				/* LkgMon AMUXSEL  */
				ocp_write_field(0x10221168, 19:0, 0x0);
				ocp_write_field(0x10221160, 9:5, 0x0);

				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10221168, 15:15, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10221058, 19:16, i);
					mdelay(10000);
				}

				ocp_write_field(0x10221168, 15:15, 0x0);

			} else if (val[0] == 14) {
				/* LL LkgMon IREF TOP */
				ocp_write_field(0x102211FC, 0:0, 0x0);
				ocp_write_field(0x102217FC, 2:0, 0x6);
				ocp_write_field(0x10221058, 19:0, 0x77777);

				/* Enable LkgMonEn  */
				ocp_write(0x10221040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10221160, 0:0, 0x1);
				ocp_write_field(0x10221164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10221C08, 2:0, 0x0);
				ocp_write_field(0x10221C08, 10:8, 0x0);
				ocp_write_field(0x10221C08, 18:16, 0x0);
				ocp_write_field(0x10221C08, 26:24, 0x0);

				/* LkgMon AMUXSEL */
				ocp_write_field(0x10221168, 19:0, 0x0);
				ocp_write_field(0x10221160, 9:5, 0x0);
				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10221160, 9:5, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10221058, 3:0, i);
					mdelay(10000);
				}

				ocp_write_field(0x10221160, 9:5, 0x0);

			} else if (val[0] == 20) {
				/* L LkgMon IREF CPU0 */
				ocp_write_field(0x102231FC, 0:0, 0x0);
				ocp_write_field(0x102237FC, 2:0, 0x6);
				ocp_write_field(0x10223058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10223040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10223160, 0:0, 0x1);
				ocp_write_field(0x10223164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10223C08, 2:0, 0x0);
				ocp_write_field(0x10223C08, 10:8, 0x0);
				ocp_write_field(0x10223C08, 18:16, 0x0);
				ocp_write_field(0x10223C08, 26:24, 0x0);

				/* LkgMon AMUXSEL */
				ocp_write_field(0x10223168, 19:0, 0x0);
				ocp_write_field(0x10223160, 9:5, 0x0);
				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10223168, 0:0, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10223058, 7:4, i);
					mdelay(10000);
				}

				ocp_write_field(0x10223168, 0:0, 0x0);

			} else if (val[0] == 21) {
				/* MP1 LkgMon IREF CPU1 */
				ocp_write_field(0x102231FC, 0:0, 0x0);
				ocp_write_field(0x102237FC, 2:0, 0x6);
				ocp_write_field(0x10223058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10223040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10223160, 0:0, 0x1);
				ocp_write_field(0x10223164, 3:0, 0xF);

				/* SPARK AMUXSEL */
				ocp_write_field(0x10223C08, 2:0, 0x0);
				ocp_write_field(0x10223C08, 10:8, 0x0);
				ocp_write_field(0x10223C08, 18:16, 0x0);
				ocp_write_field(0x10223C08, 26:24, 0x0);

				/* LkgMon AMUXSEL */
				ocp_write_field(0x10223168, 19:0, 0x0);
				ocp_write_field(0x10223160, 9:5 , 0x0);

				/* SRAMLDO: Liitle */
				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10223168, 5:5, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10223058, 11:8, i);
					mdelay(10000);
				}

				ocp_write_field(0x10223168, 5:5, 0x0);

			} else if (val[0] == 22) {
				/* L LkgMon IREF CPU2 */
				ocp_write_field(0x102231FC, 0:0, 0x0);
				ocp_write_field(0x102237FC, 2:0, 0x6);
				ocp_write_field(0x10223058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10223040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10223160, 0:0, 0x1);
				ocp_write_field(0x10223164, 3:0, 0xF);

				ocp_write_field(0x10223C08, 2:0, 0x0);
				ocp_write_field(0x10223C08, 10:8, 0x0);
				ocp_write_field(0x10223C08, 18:16, 0x0);
				ocp_write_field(0x10223C08, 26:24, 0x0);

				ocp_write_field(0x10223168, 19:0, 0x0);
				ocp_write_field(0x10223160, 9:5, 0x0);

				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10223168, 10:10, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10223058, 15:12, i);
					mdelay(10000);
				}

				ocp_write_field(0x10223168, 10:10, 0x0);

			} else if (val[0] == 23) {
				/* L LkgMon IREF CPU3 */
				ocp_write_field(0x102231FC, 0:0, 0x0);
				ocp_write_field(0x102237FC, 2:0, 0x6);
				ocp_write_field(0x10223058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10223040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10223160, 0:0, 0x1);
				ocp_write_field(0x10223164, 3:0, 0xF);

				ocp_write_field(0x10223C08, 2:0, 0x0);
				ocp_write_field(0x10223C08, 10:8, 0x0);
				ocp_write_field(0x10223C08, 18:16, 0x0);
				ocp_write_field(0x10223C08, 26:24, 0x0);

				ocp_write_field(0x10223168, 19:0, 0x0);
				ocp_write_field(0x10223160, 9:5 , 0x0);

				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10223168, 15:15, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10223058, 19:16, i);
					mdelay(10000);
				}

				ocp_write_field(0x10223168, 15:15, 0x0);

			} else if (val[0] == 24) {
				/* L LkgMon IREF TOP */
				ocp_write_field(0x102231FC, 0:0, 0x0);
				ocp_write_field(0x102237FC, 2:0, 0x6);
				ocp_write_field(0x10223058, 19:0, 0x77777);

				/* Enable LkgMonEn */
				ocp_write(0x10223040, 0x7C);
				/* Enable LkgMoninit */
				ocp_write_field(0x10223160, 0:0, 0x1);
				ocp_write_field(0x10223164, 3:0, 0xF);

				ocp_write_field(0x10223C08, 2:0  , 0x0);
				ocp_write_field(0x10223C08, 10:8 , 0x0);
				ocp_write_field(0x10223C08, 18:16, 0x0);
				ocp_write_field(0x10223C08, 26:24, 0x0);

				ocp_write_field(0x10223168, 19:0, 0x0);
				ocp_write_field(0x10223160, 9:5 , 0x0);

				_ocp_write_field(OCPPROB_BASE + 0xfac, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				_ocp_write(OCPPROB_BASE + 0x1510, ~(1<<30));

				ocp_write_field(0x10223160, 9:5, 0x1);

				for (i = 0; i < 16; i++) {
					ocp_write_field(0x10223058, 3:0, i);
					mdelay(10000);
				}

				ocp_write_field(0x10223160, 9:5, 0x0);

			}

		}

		break;
	case 4:
		/* PMUX JTAG */
		_ocp_write(OCPPIN_UDI_JTAG1, 0x331111);
		_ocp_write(OCPPIN_UDI_JTAG2, 0x1100333);
		break;
	case 5:
		/* PMUX SD CARD  */
		_ocp_write(OCPPIN_UDI_SDCARD, 0x04414440);
		break;
	case 6:
		/* AUXPMUX switch */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			if (val[0] == 0) {
				if (val[1] == 0) {
					/* disable pinmux */
					_ocp_write_field(OCPPROB_BASE + 0x1510, 30:30, 0x1);
					/* L/LL CPU AGPIO prob out volt disable */
					_ocp_write(OCPPROB_BASE + 0xfac, 0x000000ff);
				} else if (val[1] == 1) {
					/* enable pinmux */
					/* L/LL CPU AGPIO enable */
					_ocp_write_field(OCPPROB_BASE + 0x1510, 30:30, 0x0);
					/* L/LL CPU AGPIO prob out volt enable */
					_ocp_write(OCPPROB_BASE + 0xfac, 0x0000ff00);
				}
			} else if (val[0] == 1) {
				if (val[1] == 0) {
					/* disable pinmux */
					/* BIG CPU AGPIO prob out volt disable */
					_ocp_write_field(OCPPROB_BASE + 0x1890, 6:6, 0x1);
					ocp_write_field(0x102222b0, 31:31, 0x0);
				} else if (val[1] == 1) {
					/* enable pinmux */
					/* BIG CPU AGPIO enable */
					_ocp_write_field(OCPPROB_BASE + 0x1890, 6:6, 0x0);
					ocp_write_field(0x102222b0, 31:31, 0x1);
				}
			} else if (val[0] == 2) {
				if (val[1] == 0) {
					/* disable pinmux */
					_ocp_write_field(OCPPROB_BASE + 0x1120, 9:9, 0x1);
					/* GPU AGPIO prob out volt disable */
					_ocp_write(OCPPROB_BASE + 0xfd4, 0x000000ff);
				} else if (val[1] == 1) {
					/* enable pinmux */
					/* GPU AGPIO enable */
					_ocp_write_field(OCPPROB_BASE + 0x1120, 9:9, 0x0);
					/* GPU AGPIO prob out volt enable */
					_ocp_write(OCPPROB_BASE + 0xfd4, 0x0000ff00);
				}
			}
		}
		break;
	case 7:
		/* vproc L/LL/CCI, vsram */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			unsigned int rdata = 0;
			unsigned int wdata = 0;
			unsigned int volt_sram = val[1]*100;

			da9214_config_interface(0xD7, (((((val[0]*100) - 30000) + 900) / 1000) & 0x7F), 0x7F, 0);
			_ocp_write(OCPPROB_BASE + 0xf98, 0x000000ff);
		switch (volt_sram) {
		case 60000:
			rdata = 1;
			break;
		case 70000:
			rdata = 2;
			break;
		default:
			rdata = ((((volt_sram) - 90000) / 2500) + 3);
			break;
		};

		for (i = 0; i < 4; i++)
			wdata = wdata | (rdata << (i*8));

			_ocp_write(OCPPROB_BASE + 0xf9c, wdata);
			_ocp_write(OCPPROB_BASE + 0xfa0, wdata);
		}
		break;
	case 8:
		/* vproc Big, vrsam */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			da9214_config_interface(0xD9, ((((((val[0]*100) - 30000) + 900) / 1000) & 0x7F) | 0x80),
			0xFF, 0);
			BigiDVFSSRAMLDOSet(val[1]*100);
		}
		break;
	case 9:
		if (sscanf(buf, "%d %x %d %d %x", &function_id, &dvt_test_set, &dvt_test_msb,
				&dvt_test_lsb, &val[0]) == 5) {
			ocp_write_field(dvt_test_set, dvt_test_msb:dvt_test_lsb, val[0]);
			dvt_test_on = 1;
		} else if (sscanf(buf, "%d %x %d %d", &function_id, &dvt_test_set, &dvt_test_msb, &dvt_test_lsb) == 4)
			dvt_test_on = 1;

		break;
	case 10:
		if (sscanf(buf, "%d %x %d %d %x", &function_id, &dvt_test_set, &dvt_test_msb,
				&dvt_test_lsb, &val[0]) == 5) {
			_ocp_write_field(OCPPROB_BASE + dvt_test_set, dvt_test_msb:dvt_test_lsb, val[0]);
			dvt_test_on = 1;
		} else if (sscanf(buf, "%d %x %d %d", &function_id, &dvt_test_set, &dvt_test_msb, &dvt_test_lsb) == 4)
			dvt_test_on = 1;

		break;
	case 11:
		/* SPARK */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				switch (val[0]) {
				case 0:
					ocp_write_field(0x10221C04, 0:0, val[1]);
					break;
				case 1:
					ocp_write_field(0x10221C04, 1:1, val[1]);
					break;
				case 2:
					ocp_write_field(0x10221C04, 2:2, val[1]);
					break;
				case 3:
					ocp_write_field(0x10221C04, 3:3, val[1]);
					break;
				case 4:
					ocp_write_field(0x10223C04, 0:0, val[1]);
					break;
				case 5:
					ocp_write_field(0x10223C04, 1:1, val[1]);
					break;
				case 6:
					ocp_write_field(0x10223C04, 2:2, val[1]);
					break;
				case 7:
					ocp_write_field(0x10223C04, 3:3, val[1]);
					break;
				case 8:
					ocp_write_field(0x10222430, 0:0, val[1]);
					break;
				case 9:
					ocp_write_field(0x10222434, 0:0, val[1]);
					break;
				default:
					break;
			}
	case 12: /* set OCdtKi */
		if (sscanf(buf, "%d %x", &function_id, &val[0]) == 2)
			ocp_write_field(0x10222580, 19:0, val[0]);
		break;
	case 13: /* set LO */
		if (sscanf(buf, "%d %x %x %x %x", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5) {
			ocp_write(0x10222410, val[0]); /* nocpu lo trigger_en */
			ocp_write(0x10222414, val[1]); /* cpu0 lo trigger_en */
			ocp_write(0x10222418, val[2]); /* cpu1 lo trigger_en */
			ocp_write(0x10222424, val[3]); /* LO ctrl settings */
		}
		break;
	case 14: /* stop PPM to access OCP */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			if (val[0] == OCP_LL)
				ocp_opt.ocp_cluster0_flag = val[1];
			else if (val[0] == OCP_L)
				ocp_opt.ocp_cluster1_flag = val[1];
			else if (val[0] == OCP_B)
				ocp_opt.ocp_cluster2_flag = val[1];
			ocp_info("ocp_cluster%d_init = %d", val[0], val[1]);
		}
		break;
	default:
		break;
	}
}
free_page((unsigned long)buf);
return count;
}

/* hqa_test*/
static int hqa_test_proc_show(struct seq_file *m, void *v)
{
int i;

	for (i = 0; i < hqa_test; i++) {
		if (ocp_opt.ocp_cluster0_flag == 0) {
			seq_printf(m, "Cluster_0_AvgActValid  = %d\n", ocp_hqa[0][i].CGAvgValid);
			seq_printf(m, "Cluster_0_AvgAct       = %llu mA\n", ocp_hqa[0][i].CGAvg);
			seq_printf(m, "Cluster_0_AvgLkg       = %llu mA\n", ocp_hqa[0][i].AvgLkg);
		if (little_dvfs_on == 1) {
			seq_printf(m, "Cluster_0_Freq         = %d Mhz\n", dvfs_hqa[0][i].Freq);
			seq_printf(m, "Cluster_0_Vproc        = %d mV\n", dvfs_hqa[0][i].Volt);
			seq_printf(m, "Cluster_0_Thermal      = %d mC\n", dvfs_hqa[0][i].Thermal);
			seq_printf(m, "Cluster_0_CapToLkg     = %d mA\n", ocp_hqa[0][i].CapToLkg);
			seq_printf(m, "Cluster_0_CapOCCGPct   = %d %%\n", ocp_hqa[0][i].CapOCCGPct);
			seq_printf(m, "Cluster_0_CaptureValid = %d\n", ocp_hqa[0][i].CaptureValid);
			seq_printf(m, "Cluster_0_CapTotAct    = %d mA\n", ocp_hqa[0][i].CapTotAct);
			seq_printf(m, "Cluster_0_CapMAFAct    = %d mA\n", ocp_hqa[0][i].CapMAFAct);
			}
			seq_printf(m, "Cluster_0_TopRawLkg    = %u\n", ocp_hqa[0][i].TopRawLkg);
			seq_printf(m, "Cluster_0_CPU0RawLkg   = %u\n", ocp_hqa[0][i].CPU0RawLkg);
			seq_printf(m, "Cluster_0_CPU1RawLkg   = %u\n", ocp_hqa[0][i].CPU1RawLkg);
			seq_printf(m, "Cluster_0_CPU2RawLkg   = %u\n", ocp_hqa[0][i].CPU2RawLkg);
			seq_printf(m, "Cluster_0_CPU3RawLkg   = %u\n", ocp_hqa[0][i].CPU3RawLkg);
		}
		if (ocp_opt.ocp_cluster1_flag == 0) {
			seq_printf(m, "Cluster_1_AvgActValid  = %d\n", ocp_hqa[1][i].CGAvgValid);
			seq_printf(m, "Cluster_1_AvgAct       = %llu mA\n", ocp_hqa[1][i].CGAvg);
			seq_printf(m, "Cluster_1_AvgLkg       = %llu mA\n", ocp_hqa[1][i].AvgLkg);
		if (little_dvfs_on == 1) {
			seq_printf(m, "Cluster_1_Freq         = %d Mhz\n", dvfs_hqa[1][i].Freq);
			seq_printf(m, "Cluster_1_Vproc        = %d mV\n", dvfs_hqa[1][i].Volt);
			seq_printf(m, "Cluster_1_Thermal      = %d mC\n", dvfs_hqa[1][i].Thermal);
			seq_printf(m, "Cluster_1_CapToLkg     = %d mA\n", ocp_hqa[1][i].CapToLkg);
			seq_printf(m, "Cluster_1_CapOCCGPct   = %d %%\n", ocp_hqa[1][i].CapOCCGPct);
			seq_printf(m, "Cluster_1_CaptureValid = %d\n", ocp_hqa[1][i].CaptureValid);
			seq_printf(m, "Cluster_1_CapTotAct    = %d mA\n", ocp_hqa[1][i].CapTotAct);
			seq_printf(m, "Cluster_1_CapMAFAct    = %d mA\n", ocp_hqa[1][i].CapMAFAct);
			}
			seq_printf(m, "Cluster_1_TopRawLkg    = %u\n", ocp_hqa[1][i].TopRawLkg);
			seq_printf(m, "Cluster_1_CPU0RawLkg   = %u\n", ocp_hqa[1][i].CPU0RawLkg);
			seq_printf(m, "Cluster_1_CPU1RawLkg   = %u\n", ocp_hqa[1][i].CPU1RawLkg);
			seq_printf(m, "Cluster_1_CPU2RawLkg   = %u\n", ocp_hqa[1][i].CPU2RawLkg);
			seq_printf(m, "Cluster_1_CPU3RawLkg   = %u\n", ocp_hqa[1][i].CPU3RawLkg);
		}
		if (ocp_opt.ocp_cluster2_flag == 0) {
				seq_printf(m, "Cluster_2_Freq         = %d Mhz\n", dvfs_hqa[2][i].Freq);
				seq_printf(m, "Cluster_2_Vproc        = %d mV\n", dvfs_hqa[2][i].Volt);
				seq_printf(m, "Cluster_2_Thermal      = %d mC\n", dvfs_hqa[2][i].Thermal);
			if (ocp_hqa[2][i].CaptureValid == 1) {
				seq_printf(m, "Cluster_2_CGAvgValid   = %d\n", ocp_hqa[2][i].CGAvgValid);
				seq_printf(m, "Cluster_2_CGAvg        = %llu %%\n", ocp_hqa[2][i].CGAvg);
				seq_printf(m, "Cluster_2_CapToLkg     = %d mA\n", ocp_hqa[2][i].CapToLkg);
				seq_printf(m, "Cluster_2_CapOCCGPct   = %d %%\n", ocp_hqa[2][i].CapOCCGPct);
				seq_printf(m, "Cluster_2_CaptureValid = %d\n", ocp_hqa[2][i].CaptureValid);
				seq_printf(m, "Cluster_2_CapTotAct    = %d mA\n", ocp_hqa[2][i].CapTotAct);
				seq_printf(m, "Cluster_2_CapMAFAct    = %d mA\n", ocp_hqa[2][i].CapMAFAct);
				seq_printf(m, "Cluster_2_TopRawLkg    = %d\n", ocp_hqa[2][i].TopRawLkg);
				seq_printf(m, "Cluster_2_CPU0RawLkg   = %d\n", ocp_hqa[2][i].CPU0RawLkg);
				seq_printf(m, "Cluster_2_CPU1RawLkg   = %d\n", ocp_hqa[2][i].CPU1RawLkg);
			} else {
				seq_printf(m, "Cluster_2_AvgAct       = %llu mA\n", ocp_hqa[2][i].CGAvg);
			}
		}
	}

	return 0;
}

static ssize_t hqa_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int i, j, function_id, val[4], calibration, tmp, count_LL, count_L;
unsigned int freq_LL, freq_L, volt_LL, volt_L;
int Leakage, Total, ClkPct, CapMAFAct, CGAvg;
unsigned long long AvgAct, AvgLkg;
int TopRawLkg, CPU0RawLkg, CPU1RawLkg, CPU2RawLkg, CPU3RawLkg;
char *buf = _copy_from_user_for_proc(buffer, count);
ktime_t now, delta;
unsigned char da9214;
unsigned int cnt, cnt0, cnt1, cnt2, cnt3;

	if (!buf)
		return -EINVAL;

if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) > 0) {
		switch (function_id) {
		case 1:
			/* LL/L workload calibration */
			if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5) {
				hqa_test = val[0];
				if (hqa_test > NR_HQA)
					hqa_test = NR_HQA;

					if (cpu_online(8))
						sched_setaffinity(0, cpumask_of(8));
					else if (cpu_online(9))
						sched_setaffinity(0, cpumask_of(9));
					ocp_opt.ocp_cluster0_flag = 0;
					ocp_opt.ocp_cluster1_flag = 0;
					do_ocp_test_on = 1;
					little_dvfs_on = 0;
					LittleLowerPowerOff(0, 1);
					LittleLowerPowerOff(1, 1);

					calibration = 0;
					freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000;
					volt_LL = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100;
					freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000;
					volt_L = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100;
					LittleOCPDVFSSet(0, freq_LL, volt_LL);
					LittleOCPDVFSSet(1, freq_L, volt_L);
					count_LL = freq_LL * val[1] * 10;
					count_L = freq_L * val[2] * 10;
					ocp_info("L_freq=%u Mhz/L_volt=%u mV/LL_freq=%u Mhz/LL_volt=%u mV\n",
							freq_L, volt_L, freq_LL, volt_LL);

					for (i = 0; i < hqa_test; i++) {
						/* This test must disable PPM*/
						/* now = ktime_get(); */
						OCPMcusysPwrSet(); /* MCUSYS power set*/
						LittleOCPAvgPwr_HQA(0, 1, count_LL);
						LittleOCPAvgPwr_HQA(1, 1, count_L);
						udelay(val[3]);

						LittleOCPCapture(0, 1, 1, 0, 15);
						LittleOCPCapture(1, 1, 1, 0, 15);
						/*CL0OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg,
								&CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
						ocp_hqa[0][i].TopRawLkg = TopRawLkg;
						ocp_hqa[0][i].CPU0RawLkg = CPU0RawLkg;
						ocp_hqa[0][i].CPU1RawLkg = CPU1RawLkg;
						ocp_hqa[0][i].CPU2RawLkg = CPU2RawLkg;
						ocp_hqa[0][i].CPU3RawLkg = CPU3RawLkg;
						ocp_hqa[0][i].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);
*/
						CL1OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg,
								&CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
						ocp_hqa[1][i].TopRawLkg = TopRawLkg;
						ocp_hqa[1][i].CPU0RawLkg = CPU0RawLkg;
						ocp_hqa[1][i].CPU1RawLkg = CPU1RawLkg;
						ocp_hqa[1][i].CPU2RawLkg = CPU2RawLkg;
						ocp_hqa[1][i].CPU3RawLkg = CPU3RawLkg;
						ocp_hqa[1][i].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);

						LittleOCPAvgPwrGet_HQA(1, &AvgLkg, &AvgAct);
						ocp_hqa[1][i].CGAvgValid = ocp_read_field(MP1_OCP_DBG_STAT, 31:31);
						ocp_hqa[1][i].CGAvg = AvgAct;
						ocp_hqa[1][i].AvgLkg = AvgLkg;

						LittleOCPAvgPwrGet_HQA(0, &AvgLkg, &AvgAct);
						ocp_hqa[0][i].CGAvgValid = ocp_read_field(MP0_OCP_DBG_STAT, 31:31);
						ocp_hqa[0][i].CGAvg = AvgAct;
						ocp_hqa[0][i].AvgLkg = AvgLkg;

						/* MCUSYS power get*/
						OCPMcusysPwrCapture(&cnt, &cnt0, &cnt1, &cnt2, &cnt3);
						ocp_hqa[0][i].TopRawLkg = cnt;
						ocp_hqa[0][i].CPU0RawLkg = cnt0;
						ocp_hqa[0][i].CPU1RawLkg = cnt1;
						ocp_hqa[0][i].CPU2RawLkg = cnt2;
						ocp_hqa[0][i].CPU3RawLkg = cnt3;
						/* delta = ktime_sub(ktime_get(), now); */

						/* ocp_info("Cluster 0/1 delta=%lld us\n", ktime_to_us(delta)); */
					}
				}
				LittleLowerPowerOn(1, 0);
				LittleLowerPowerOn(0, 0);
				do_ocp_test_on = 0;
				break;
		case 2:
				if (sscanf(buf, "%d %d %d %d", &function_id, &val[0], &val[1], &val[2]) == 4) {
					hqa_test = val[0];
					if (hqa_test > NR_HQA)
						hqa_test = NR_HQA;

					ocp_opt.ocp_cluster2_flag = 0;
					do_ocp_test_on = 1;
					if (cpu_online(4))
						sched_setaffinity(0, cpumask_of(4));
					else if (cpu_online(0))
						sched_setaffinity(0, cpumask_of(0));

					calibration = 0;
					BigOCPClkAvg(1, val[1]);
					for (j = 0; j < hqa_test; j++) {
						now = ktime_get();

						tmp = (val[2] - calibration);
					if (tmp < 0)
						tmp = 0;
					else
						udelay(tmp);
						BigOCPCapture(1, 1, 0, 15);

						da9214_read_interface(0xd9, &da9214, 0x7f, 0);
						dvfs_hqa[2][j].Volt = DA9214_STEP_TO_MV((da9214 & 0x7f));

					if (ocp_read_field(0x10222470, 0:0) == 1)
						dvfs_hqa[2][j].Freq = ((BigiDVFSSWAvgStatus() * 25)/100);
					else
						dvfs_hqa[2][j].Freq = BigiDVFSPllGetFreq();

						dvfs_hqa[2][j].Thermal = get_immediate_big_wrap();

						BigOCPCaptureStatus(&Leakage, &Total, &ClkPct);
						ocp_hqa[2][j].CapToLkg = Leakage;
						ocp_hqa[2][j].CapTotAct = Total;
						ocp_hqa[2][j].CapOCCGPct = ClkPct;
						ocp_hqa[2][j].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);

						BigOCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg);
						ocp_hqa[2][j].TopRawLkg = TopRawLkg;
						ocp_hqa[2][j].CPU0RawLkg = CPU0RawLkg;
						ocp_hqa[2][j].CPU1RawLkg = CPU1RawLkg;

						BigOCPMAFAct(&CapMAFAct);
						ocp_hqa[2][j].CapMAFAct = CapMAFAct;

						BigOCPClkAvgStatus(&CGAvg);
						ocp_hqa[2][j].CGAvgValid = ocp_read_field(OCPAPBSTATUS04, 27:27);
						ocp_hqa[2][j].CGAvg = (unsigned long long)CGAvg;

						delta = ktime_sub(ktime_get(), now);
						calibration = ktime_to_us(delta) - tmp;

						ocp_info("Cluster 2 calibration=%d us\n", calibration);
					}

				}
				break;
		case 3:
			if (sscanf(buf, "%d %d %d %d", &function_id, &val[0], &val[1], &val[2]) == 4) {
				hqa_test = val[0];
					if (hqa_test > NR_HQA)
						hqa_test = NR_HQA;

					ocp_opt.ocp_cluster2_flag = 0;
					do_ocp_test_on = 0;
					calibration = 0;
					for (j = 0; j < hqa_test; j++) {
						now = ktime_get();

						da9214_read_interface(0xd9, &da9214, 0x7f, 0);
						dvfs_hqa[2][j].Volt = DA9214_STEP_TO_MV((da9214 & 0x7f));

					if (ocp_read_field(0x10222470, 0:0) == 1)
						dvfs_hqa[2][j].Freq = ((BigiDVFSSWAvgStatus() * 25)/100);
					else
						dvfs_hqa[2][j].Freq = BigiDVFSPllGetFreq();

						dvfs_hqa[2][j].Thermal = get_immediate_big_wrap();

						ocp_hqa[2][j].CaptureValid = 0;
						ocp_hqa[2][j].CGAvg = BigOCPAvgPwrGet(val[1]);
						tmp = (val[2] - calibration);
					if (tmp < 0)
						tmp = 0;
					else
						udelay(tmp);

						delta = ktime_sub(ktime_get(), now);
						calibration = ktime_to_us(delta) - tmp;
						ocp_info("Cluster 2 calibration=%d us\n", calibration);

					}
				}
				break;
		case 4:
			/* LL/L workload calibration with freq and volt info*/
			if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5) {
				hqa_test = val[0];
				if (hqa_test > NR_HQA)
					hqa_test = NR_HQA;

					ocp_opt.ocp_cluster0_flag = 0;
					ocp_opt.ocp_cluster1_flag = 0;
					if (cpu_online(8))
						sched_setaffinity(0, cpumask_of(8));
					else if (cpu_online(9))
						sched_setaffinity(0, cpumask_of(9));

					do_ocp_test_on = 1;
					little_dvfs_on = 1;
					LittleLowerPowerOff(0, 1);
					LittleLowerPowerOff(1, 1);

					calibration = 0;

					for (i = 0; i < hqa_test; i++) {
						now = ktime_get();

						freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000;
						volt_LL = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100;
						freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000;
						volt_L = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100;
						LittleOCPDVFSSet(0, freq_LL, volt_LL);
						LittleOCPDVFSSet(1, freq_L, volt_L);
						count_LL = freq_LL * val[1] * 10;
						count_L = freq_L * val[2] * 10;
						ocp_info("L_freq=%u Mhz/L_volt=%u mV/LL_freq=%u Mhz/LL_volt=%u mV\n",
								freq_L, volt_L, freq_LL, volt_LL);

						OCPMcusysPwrSet(); /* MCUSYS power set*/
						LittleOCPAvgPwr_HQA(0, 1, count_LL);
						LittleOCPAvgPwr_HQA(1, 1, count_L);

						dvfs_hqa[0][i].Freq = freq_LL;
						dvfs_hqa[0][i].Volt = volt_LL;
						dvfs_hqa[1][i].Freq = freq_L;
						dvfs_hqa[1][i].Volt = volt_L;

						dvfs_hqa[0][i].Thermal = get_immediate_cpuLL_wrap();
						dvfs_hqa[1][i].Thermal = get_immediate_cpuL_wrap();

						udelay(val[3]);

						LittleOCPCapture(0, 1, 1, 0, 15);
						LittleOCPCapture(1, 1, 1, 0, 15);

						LittleOCPCaptureGet(1, &Leakage, &Total, &ClkPct);
						ocp_hqa[1][i].CapToLkg = Leakage;
						ocp_hqa[1][i].CapTotAct = Total;
						ocp_hqa[1][i].CapOCCGPct = ClkPct;
						ocp_hqa[1][i].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);

						LittleOCPCaptureGet(0, &Leakage, &Total, &ClkPct);
						ocp_hqa[0][i].CapToLkg = Leakage;
						ocp_hqa[0][i].CapTotAct = Total;
						ocp_hqa[0][i].CapOCCGPct = ClkPct;
						ocp_hqa[0][i].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);
/*
						CL0OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg,
								&CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
						ocp_hqa[0][i].TopRawLkg = TopRawLkg;
						ocp_hqa[0][i].CPU0RawLkg = CPU0RawLkg;
						ocp_hqa[0][i].CPU1RawLkg = CPU1RawLkg;
						ocp_hqa[0][i].CPU2RawLkg = CPU2RawLkg;
						ocp_hqa[0][i].CPU3RawLkg = CPU3RawLkg;
						ocp_hqa[0][i].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);
*/
						CL1OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg,
								&CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
						ocp_hqa[1][i].TopRawLkg = TopRawLkg;
						ocp_hqa[1][i].CPU0RawLkg = CPU0RawLkg;
						ocp_hqa[1][i].CPU1RawLkg = CPU1RawLkg;
						ocp_hqa[1][i].CPU2RawLkg = CPU2RawLkg;
						ocp_hqa[1][i].CPU3RawLkg = CPU3RawLkg;
						ocp_hqa[1][i].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);

						LittleOCPMAFAct(1, &CapMAFAct);
						ocp_hqa[1][i].CapMAFAct = CapMAFAct;

						LittleOCPMAFAct(0, &CapMAFAct);
						ocp_hqa[0][i].CapMAFAct = CapMAFAct;

						LittleOCPAvgPwrGet_HQA(1, &AvgLkg, &AvgAct);
						ocp_hqa[1][i].CGAvgValid = ocp_read_field(MP1_OCP_DBG_STAT, 31:31);
						ocp_hqa[1][i].CGAvg = AvgAct;
						ocp_hqa[1][i].AvgLkg = AvgLkg;

						LittleOCPAvgPwrGet_HQA(0, &AvgLkg, &AvgAct);
						ocp_hqa[0][i].CGAvgValid = ocp_read_field(MP0_OCP_DBG_STAT, 31:31);
						ocp_hqa[0][i].CGAvg = AvgAct;
						ocp_hqa[0][i].AvgLkg = AvgLkg;
						/* MCUSYS power get*/
						OCPMcusysPwrCapture(&cnt, &cnt0, &cnt1, &cnt2, &cnt3);
						ocp_hqa[0][i].TopRawLkg = cnt;
						ocp_hqa[0][i].CPU0RawLkg = cnt0;
						ocp_hqa[0][i].CPU1RawLkg = cnt1;
						ocp_hqa[0][i].CPU2RawLkg = cnt2;
						ocp_hqa[0][i].CPU3RawLkg = cnt3;

						delta = ktime_sub(ktime_get(), now);

						ocp_info("Cluster 0/1 delta=%lld us\n", ktime_to_us(delta));
					}
				}
				LittleLowerPowerOn(1, 0);
				LittleLowerPowerOn(0, 0);
				do_ocp_test_on = 0;
				break;
		default:
			break;
		}
}
	free_page((unsigned long)buf);
	return count;
}


static int ocp_stress_test_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", do_ocp_stress_test);

	return 0;
}

static ssize_t ocp_stress_test_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int do_stress,  big_pwr_limit;
	/*unsigned int freq_L, volt_L, freq_LL, volt_LL, LL_pwr_limit, L_pwr_limit;*/
	int rc;
	/*int i;*/
	unsigned long long count1;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;
	rc = kstrtoint(buf, 10, &do_stress);
	if (rc < 0)
		ocp_err("echo 0/1 > /proc/ocp/ocp_stress_test\n");
	else
		do_ocp_stress_test = do_stress;

	if (do_ocp_stress_test == 0) {
		/*if (ocp_opt.ocp_cluster0_enable == 1) {
			freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000;
			volt_LL = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100;
			LittleOCPDVFSSet(0, freq_LL, volt_LL);
			LittleOCPSetTarget(0, 127000);
			LittleLowerPowerOn(0, 0);
		}
		if (ocp_opt.ocp_cluster1_enable == 1) {
			freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000;
			volt_L = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100;
			LittleOCPDVFSSet(1, freq_L, volt_L);
			LittleOCPSetTarget(1, 127000);
			LittleLowerPowerOn(1, 0);
		}*/
		if (ocp_opt.ocp_cluster2_enable == 1) {
			BigOCPSetTarget(3, 127000);
			ocp_opt.ocp_cluster2_flag = 1;
		}
		ocp_info("stress test: stop\n");
	}

count1 = 0;




while (do_ocp_stress_test == 1) {
	if (ocp_opt.ocp_cluster2_enable == 1) {
		big_pwr_limit = jiffies % 10000;
		ocp_opt.ocp_cluster2_flag = 0;
		BigOCPSetTarget(3, big_pwr_limit);
		ocp_info("stress test: big power limit=%u\n", big_pwr_limit);
	} else
		ocp_info("stress test: cluster 2 (Big) is off\n");

	mdelay(2000);
/*
	if (ocp_opt.ocp_cluster0_enable == 1) {
		LittleLowerPowerOff(0, 1);
		freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000;
		volt_LL = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100;
		LittleOCPDVFSSet(0, freq_LL, volt_LL);
		LL_pwr_limit = jiffies % 2000;
		LittleOCPSetTarget(0, LL_pwr_limit);

		ocp_info("stress test: LL_freq=%u Mhz, LL_volt=%u mV, L_power limit=%u\n",
				freq_LL, volt_LL, LL_pwr_limit);
	} else
		ocp_info("stress test: cluster 0 (LL) is off\n");

	if (ocp_opt.ocp_cluster1_enable == 1) {
		LittleLowerPowerOff(1, 1);
		freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000;
		volt_L = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100;
		LittleOCPDVFSSet(1, freq_L, volt_L);
		L_pwr_limit = jiffies % 4000;
		LittleOCPSetTarget(1, L_pwr_limit);
		ocp_info("stress test: L_freq=%u Mhz, L_volt=%u mV, LL_power limit=%u\n",
				freq_L, volt_L, L_pwr_limit);
	} else
		ocp_info("stress test: cluster 1 (L) is off\n");

for (i = 0; i < 2000; i++) {
	if (ocp_opt.ocp_cluster0_enable == 1) {
		freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL)/1000;
		volt_LL = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL)/100;
		LittleOCPDVFSSet(0, freq_LL, volt_LL);
	}
	if (ocp_opt.ocp_cluster1_enable == 1) {
		freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L)/1000;
		volt_L = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L)/100;
		LittleOCPDVFSSet(1, freq_L, volt_L);
	}
		mdelay(1);
}
*/
		ocp_info("stress test: count = %llu\n", count1++);
	}

	free_page((unsigned long)buf);
	return count;
}



/* ocp_debug */
static int ocp_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "HW_API_DEBUG_ON       = %d\n", HW_API_DEBUG_ON);
	seq_printf(m, "IRQ_Debug_on          = %d\n", IRQ_Debug_on);
	seq_printf(m, "Reg_Debug_on          = %d\n", Reg_Debug_on);
	seq_printf(m, "BIG_OCP_HW            = %d\n", BIG_OCP_ON);
if (LITTLE_OCP_ON)
	seq_printf(m, "LITTLE_OCP_HW         = %d\n", LITTLE_OCP_ON);
return 0;
}

static ssize_t ocp_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int dbg_lv, rc;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;
rc = kstrtoint(buf, 10, &dbg_lv);
if (rc < 0)
	ocp_err("echo dbg_lv (dec) > /proc/ocp/ocp_debug\n");
else
	OCP_DEBUG_FLAG = dbg_lv;


if (OCP_DEBUG_FLAG == 0) {
	HW_API_DEBUG_ON = 0;
	IRQ_Debug_on = 0;
	Reg_Debug_on = 0;
	BIG_OCP_ON = 1;
	LITTLE_OCP_ON = 0;
} else if (OCP_DEBUG_FLAG == 1)
	HW_API_DEBUG_ON = 1;
else if  (OCP_DEBUG_FLAG == 2)
	IRQ_Debug_on = 1;
else if  (OCP_DEBUG_FLAG == 3)
	Reg_Debug_on = 1;
else if  (OCP_DEBUG_FLAG == 4)
	BIG_OCP_ON = 0;
else if  (OCP_DEBUG_FLAG == 5)
	LITTLE_OCP_ON = 1;
else if  (OCP_DEBUG_FLAG == 6)
	BIG_OCP_ON = 1;



free_page((unsigned long)buf);
return count;
}

#define PROC_FOPS_RW(name)                          \
static int name ## _proc_open(struct inode *inode, struct file *file)   \
{                                   \
return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
}                                   \
static const struct file_operations name ## _proc_fops = {      \
.owner          = THIS_MODULE,                  \
.open           = name ## _proc_open,               \
.read           = seq_read,                 \
.llseek         = seq_lseek,                    \
.release        = single_release,               \
.write          = name ## _proc_write,              \
}

#define PROC_FOPS_RO(name)                          \
static int name ## _proc_open(struct inode *inode, struct file *file)   \
{                                   \
return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
}                                   \
static const struct file_operations name ## _proc_fops = {      \
.owner          = THIS_MODULE,                  \
.open           = name ## _proc_open,               \
.read           = seq_read,                 \
.llseek         = seq_lseek,                    \
.release        = single_release,               \
}

#define PROC_ENTRY(name)    {__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(ocp_cluster0_enable);
PROC_FOPS_RW(ocp_cluster1_enable);
PROC_FOPS_RW(ocp_cluster0_interrupt);
PROC_FOPS_RW(ocp_cluster1_interrupt);
PROC_FOPS_RW(ocp_cluster0_capture);
PROC_FOPS_RW(ocp_cluster1_capture);

PROC_FOPS_RW(ocp_cluster2_enable);
PROC_FOPS_RW(ocp_cluster2_interrupt);
PROC_FOPS_RW(ocp_cluster2_capture);
PROC_FOPS_RW(ocp_debug);
PROC_FOPS_RW(dreq_function_test);
PROC_FOPS_RW(dvt_test);
PROC_FOPS_RW(hqa_test);
PROC_FOPS_RW(ocp_stress_test);

static int _create_procfs(void)
{
struct proc_dir_entry *dir = NULL;
int i;

struct pentry {
const char *name;
const struct file_operations *fops;
};

const struct pentry entries[] = {
PROC_ENTRY(ocp_cluster0_enable),
PROC_ENTRY(ocp_cluster1_enable),
PROC_ENTRY(ocp_cluster0_interrupt),
PROC_ENTRY(ocp_cluster1_interrupt),
PROC_ENTRY(ocp_cluster0_capture),
PROC_ENTRY(ocp_cluster1_capture),
PROC_ENTRY(ocp_cluster2_enable),
PROC_ENTRY(ocp_cluster2_interrupt),
PROC_ENTRY(ocp_cluster2_capture),
PROC_ENTRY(ocp_debug),
PROC_ENTRY(dreq_function_test),
PROC_ENTRY(dvt_test),
PROC_ENTRY(hqa_test),
PROC_ENTRY(ocp_stress_test),
};

dir = proc_mkdir("ocp", NULL);

if (!dir) {
	ocp_err("fail to create /proc/ocp @ %s()\n", __func__);
	return -ENOMEM;
}

for (i = 0; i < ARRAY_SIZE(entries); i++) {
	if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
		ocp_err("%s(), create /proc/ocp/%s failed\n", __func__, entries[i].name);
}

return 0;
}

#endif
/* CONFIG_PROC_FS */

/*
* Module driver
*/
static int __init ocp_init(void)
{
int err = 0;

#if 1
struct device_node *node = NULL;
node = of_find_compatible_node(NULL, NULL, "mediatek,ocp_cfg");
	if (!node) {
		ocp_err("error: cannot find node OCP_NODE!\n");
		BUG();
	}

ocp_base = of_iomap(node, 0); /* 0x10220000 0x4000, OCP */
ocppin_base = of_iomap(node, 1); /* 0x10005000 0x1000, DFD,UDI pinmux reg */
ocpprob_base = of_iomap(node, 2); /* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */
ocpefus_base = of_iomap(node, 3); /* 0x10206000 0x1000, Big eFUSE register */

	if (!ocppin_base || !ocpprob_base || !ocpefus_base) {
		ocp_err("OCP get some base NULL.\n");
		BUG();
	}

#if OCP_ON
/* turn on OCP in hotplug stage */
BIG_OCP_ON = 1;
LITTLE_OCP_ON = 0;
#endif

/*get ocp irq num*/
ocp2_irq0_number = irq_of_parse_and_map(node, 0);
ocp2_irq1_number = irq_of_parse_and_map(node, 1);

if (LITTLE_OCP_ON) {
	ocp0_irq0_number = irq_of_parse_and_map(node, 2);
	ocp0_irq1_number = irq_of_parse_and_map(node, 3);
	ocp1_irq0_number = irq_of_parse_and_map(node, 4);
	ocp1_irq1_number = irq_of_parse_and_map(node, 5);
}
#endif




/* register platform device/driver */
err = platform_device_register(&ocp_pdev);

if (err) {
	ocp_err("fail to register OCP device @ %s()\n", __func__);
	goto out;
}

err = platform_driver_register(&ocp_pdrv);
if (err) {
	ocp_err("%s(), OCP driver callback register failed..\n", __func__);
	return err;
}


#ifdef CONFIG_PROC_FS
/* init proc */
if (_create_procfs()) {
	err = -ENOMEM;
	goto out;
}
#endif
/* CONFIG_PROC_FS */

out:
return err;
}

static void __exit ocp_exit(void)
{
ocp_info("OCP de-initialization\n");
platform_driver_unregister(&ocp_pdrv);
platform_device_unregister(&ocp_pdev);
}

module_init(ocp_init);
module_exit(ocp_exit);

MODULE_DESCRIPTION("MediaTek OCP Driver v0.1");
MODULE_LICENSE("GPL");

#undef __MT_OCP_C__
