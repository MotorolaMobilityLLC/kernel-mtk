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

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include "mach/irqs.h"
#include "mt_ocp.h"
#include "mt_idvfs.h"

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



/*add by ue, zzz */
#include <mt-plat/sync_write.h>

struct cpu_opp_info {
	int freq;
	int pll;
	int ckdiv;
	int postdiv;
};

struct cpu_info {
	const char *name;
	struct cpu_opp_info cpu_opp_info_LL[8];
	struct cpu_opp_info cpu_opp_info_L[8];
	int vmin_hi;
	int vmin_lo;
};

/* adb command list
echo 6 0 > /proc/ocp/dvt_test //disable AUXPUMX
echo 6 1 > /proc/ocp/dvt_test //enable  AUXPUMX(L/LL Sram LDO only)

echo 7 > 1000 1100 /proc/ocp/dvt_test // L,LL Vproc, Vsram
echo 8 > 1000 1100 /proc/ocp/dvt_test // L,LL Vproc, Vsram

echo 9 7 7 > /proc/ocp/dvt_test // LL, L oppidx freq
echo 10 7 > /proc/ocp/dvt_test // big oppidx freq
*/

const unsigned int big_opp[] = { 2288, 2093, 1495, 1001, 897, 806, 750, 350 };

/* freq, pll, clkdiv, posdiv */
struct cpu_info cpu_info_fy = {
"FY",
{{ 1001, 0x40134000, 0x8, 0x1 }, /* LL */
{  910, 0x40118000, 0x8, 0x1 },
{  819, 0x400FC000, 0x8, 0x1 },
{  689, 0x400D4000, 0x8, 0x1 },
{  598, 0x400B8000, 0x8, 0x1 },
{  494, 0x40130000, 0x8, 0x2 },
{  338, 0x400D0000, 0x8, 0x2 },
{  156, 0x400C0000, 0x8, 0x3 },
},
{ { 1807, 0x40116000, 0x8, 0x0 }, /* L */
{ 1651, 0x400FE000, 0x8, 0x0 },
{ 1495, 0x400E6000, 0x8, 0x0 },
{ 1196, 0x400B8000, 0x8, 0x0 },
{ 1027, 0x4013C000, 0x8, 0x1 },
{  871, 0x4010C000, 0x8, 0x1 },
{  663, 0x400CC000, 0x8, 0x1 },
{  286, 0x400B0000, 0x8, 0x2 },
},
0,
0,
};
/* zzz */


static unsigned int func_lv_mask;
static unsigned int IRQ_Debug_on;
static unsigned int Reg_Debug_on;
static unsigned int ocp_cluster0_enable;
static unsigned int ocp_cluster1_enable;
static unsigned int ocp_cluster2_enable;
static unsigned int big_dreq_enable;
static unsigned int little_dreq_enable;
static unsigned int dvt_test_on;




/* OCP ADDR */
#ifdef CONFIG_OF
/* 0x10220000 */
void __iomem *ocp_base;
/* 279+50=329 */
static int ocp2_irq0_number;
/* 280+50=330 */
static int ocp2_irq1_number;
/* 282+50=332  LL = MP0 */
static int ocp0_irq0_number;
/* 283+50=333  */
static int ocp0_irq1_number;
/* 284+50=334  L = MP1  */
static int ocp1_irq0_number;
/* 285+50=335  */
static int ocp1_irq1_number;
#endif

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

/* BIG CPU */
int BigOCPConfig(int VOffInmV, int VStepInuV)
{
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

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCONFIG, VOffInmV, VStepInuV, 0);

}


int BigOCPSetTarget(int OCPMode, int Target)
{

if ((Target < 0) || (Target > 127000)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("Target must be 0 ~ 127000 mA)\n");

return -1;
}

if (!((OCPMode == OCP_ALL) || (OCPMode == OCP_FPI) || (OCPMode == OCP_OCPI))) {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Invalid OCP Mode!\n");

return -1;
}

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPSETTARGET, OCPMode, Target, 0);

}


int BigOCPEnable(int OCPMode, int Units, int ClkPctMin, int FreqPctMin)
{

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

if (Units == OCP_MA)
		return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPENABLE0, OCPMode, ClkPctMin, FreqPctMin);
else if (Units == OCP_MW)
		return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPENABLE1, OCPMode, ClkPctMin, FreqPctMin);
else {
		if (HW_API_RET_DEBUG_ON)
			ocp_err("Units != OCP_mA/mW (0/1)");
return -1;
}


}


void BigOCPDisable(void)
{

mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPDISABLE, 0, 0, 0);

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
Temp = ocp_read(OCPAPBSTATUS00);
*Value1 = (Temp & 0x00FF0000) >> 16;
*Value0 = Temp & 0X000000FF;
	if (HW_API_DEBUG_ON)
		ocp_info("BIG IRQ1:IRQ0=%x:%x STAT00=%x\n", *Value1, *Value0, Temp);

}


int BigOCPCapture(int EnDis, int Edge, int Count, int Trig)
{

if (EnDis == OCP_DISABLE)
		return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCAPTURE0, Edge, Count, Trig);

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

if ((EnDis == OCP_DISABLE) || (EnDis == OCP_ENABLE))
	return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCAPTURE1, Edge, Count, Trig);
else {
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
if (ocp_read_field(OCPAPBSTATUS01, 0:0) == 1) {
	Temp = ocp_read(OCPAPBSTATUS01);
	/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA/mW) */
	*Leakage = ((Temp & 0x0FFFFF00) * 1000) >> 20;

	/* CapOCCGPct -> % */
	*ClkPct = (100*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;

	/* CapTotAct:  Q8.12 -> integer  */
	Temp = ocp_read(OCPAPBSTATUS02);
	*Total = ((Temp & 0x000FFFFF) * 1000) >> 12;
	if (HW_API_DEBUG_ON)
		ocp_info("ocp: big total_pwr=%d Lk_pwr=%d CG=%d\n", *Total, *Leakage, *ClkPct);

return 1;
} else {
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

ret = mt_secure_call_ocp(MTK_SIP_KERNEL_BIGOCPCLKAVG, EnDis, CGAvgSel, 0);

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: En=%x CGASel=%x CFG01=%x\n", EnDis, CGAvgSel, ocp_read(OCPAPBCFG00));

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
if (ocp_read_field(OCPAPBSTATUS04, 27:27) == 1) {
	Temp = ocp_read(OCPAPBSTATUS04);

/* CGAvg: shfit 11 bit -> UQ4.12  -> xx%  */
*CGAvg = ((((Temp & 0x07FFF800) >> 23) + 1) * 100)/16;

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
	*CapMAFAct = (ocp_read_field(OCPAPBSTATUS03, 19:0) * 1000) >> 12; /* mA(mW)*/
else
	*CapMAFAct = 0x0;

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: big CapMAFAct=%d\n",  *CapMAFAct);

return 0;
}

/* Little CPU */
int LittleOCPConfig(int VOffInmV, int VStepInuV)
{

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

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPCONFIG, VOffInmV, VStepInuV, 0);

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
return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPSETTARGET, Cluster, Target, 0);
}


int LittleOCPEnable(int Cluster, int Units, int ClkPctMin)
{

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

return mt_secure_call_ocp(MTK_SIP_KERNEL_LITTLEOCPENABLE, Cluster, Units, ClkPctMin);

}


int LittleOCPDisable(int Cluster)
{

if (!((Cluster == OCP_LL) || (Cluster == OCP_L))) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter Cluster OCP_LL/OCP_L must be 0/1\n");

return -1;
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
		/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA, mW) */
		*Leakage = ((Temp & 0x0FFFFF00) * 1000) >> 20;
		/* CapOCCGPct -> % */
		*ClkPct = (100*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;
		Temp = ocp_read((MP0_OCP_CAP_STATUS01));
		/* CapTotAct:  Q8.12 -> integer (mA, mW) */
		*Total = ((Temp & 0x000FFFFF) * 1000) >> 12;

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
		/* CapTotLkg: shit 8 bit -> Q8.12 -> integer  (mA, mW) */
		*Leakage = ((Temp & 0x0FFFFF00) * 1000) >> 20;
		/* CapOCCGPct -> % */
		*ClkPct = (100*(((Temp & 0x000000F0) >> 4) + 1)) >> 4;
		Temp = ocp_read((MP1_OCP_CAP_STATUS01));
		/* CapTotAct:  Q8.12 -> integer (mA, mW) */
		*Total = ((Temp & 0x000FFFFF) * 1000) >> 12;

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

if (Cluster == OCP_LL) {
	ocp_write_field(MP0_OCP_GENERAL_CTRL, 1:1, EnDis);
	ocp_write_field(MP0_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP0_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP0_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP0_OCP_DBG_STAT, 0:0));
	}
else if (Cluster == OCP_L) {
	ocp_write_field(MP1_OCP_GENERAL_CTRL, 1:1, EnDis);
	ocp_write_field(MP1_OCP_DBG_IFCTRL, 1:1, EnDis);
	ocp_write_field(MP1_OCP_DBG_IFCTRL1, 21:0, Count);
	ocp_write_field(MP1_OCP_DBG_STAT, 0:0, ~ocp_read_field(MP1_OCP_DBG_STAT, 0:0));
}

return 0;
}

int LittleOCPAvgPwrGet(int Cluster, unsigned long long *AvgLkg, unsigned long long *AvgAct)
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
		*CapMAFAct = (ocp_read_field(MP0_OCP_CAP_STATUS02, 19:0) * 1000) >> 12; /* mA(mW)*/

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL0 MAFAct=%d\n", *CapMAFAct);

	} else {
		*CapMAFAct = 0x0;

	}

} else if (Cluster == OCP_L) {
	if (ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0) == 1) {
		*CapMAFAct = (ocp_read_field(MP1_OCP_CAP_STATUS02, 19:0) * 1000) >> 12; /* mA(mW)*/

	if (HW_API_DEBUG_ON)
		ocp_info("ocp: CL1 MAFAct=%d\n", *CapMAFAct);

	} else {
		*CapMAFAct = 0x0;

	}
	}
return 0;
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

return mt_secure_call_ocp(MTK_SIP_KERNEL_BIGDREQHWEN, VthHi, VthLo, 0);

}


int BigDREQSWEn(int Value)
{

if ((Value < 0) || (Value > 1)) {
	if (HW_API_RET_DEBUG_ON)
		ocp_err("parameter EnDis must be 1/0\n");

return -1;
}

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

ocp_info("OCP Initial.\n");


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
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_cluster2_enable);
return 0;
}

static ssize_t ocp_cluster2_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[5], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) > 0) {
	switch (function_id) {
	case 3:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = BigOCPConfig(val[0], val[1]);
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = BigOCPSetTarget(val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d %d", &function_id, &val[0], &val[1], &val[2], &val[3]) == 5)
			ret = BigOCPEnable(val[0], val[1], val[2], val[3]);

		if (ret == 0)
			ocp_cluster2_enable = 1;

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

		if (ret == 0)
			ocp_cluster2_enable = 0;

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
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_cluster0_enable);
return 0;
}

static ssize_t ocp_cluster0_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[3], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
	switch (function_id) {
	case 4:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPConfig(val[0], val[1]);
		break;
	case 3:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
			ret = LittleOCPSetTarget(0, val[0]);
		break;
	case 2:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPDVFSSet(0, val[0], val[1]);
		break;
	case 1:
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			ret = LittleOCPEnable(0, val[0], val[1]);

		if (ret == 0)
			ocp_cluster0_enable = 1;
		break;
	case 0:
		ret = LittleOCPDisable(0);
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

		if (ret == 0)
			ocp_cluster0_enable = 0;

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
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_cluster1_enable);
return 0;
}

static ssize_t ocp_cluster1_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
int function_id, val[3], ret = 0;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) > 0) {
		switch (function_id) {
		case 4:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = LittleOCPConfig(val[0], val[1]);
			break;
		case 3:
			if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2)
				ret = LittleOCPSetTarget(1, val[0]);
			break;
		case 2:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = LittleOCPDVFSSet(1, val[0], val[1]);
			break;
		case 1:
			if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
				ret = LittleOCPEnable(1, val[0], val[1]);

				if (ret == 0)
					ocp_cluster1_enable = 1;

			break;
		case 0:
			ret = LittleOCPDisable(1);
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

			if (ret == 0)
				ocp_cluster1_enable = 0;

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
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_cluster2_enable);
seq_puts(m, "Cluster 2 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[2].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[2].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[2].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[2].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[2].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[2].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[2].CapTotAct);
/* seq_printf(m, "CapMAFAct  = %d\n", ocp_status[2].CapMAFAct); */
seq_printf(m, "CGAvgValid   = %d\n", ocp_status[2].CGAvgValid);
seq_printf(m, "CGAvg        = %llu %%\n", ocp_status[2].CGAvg);
/*
seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[2].TopRawLkg);
seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[2].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[2].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[2].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[2].CPU3RawLkg);
*/
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
/*	 case 3:
BigOCPIntStatus(&val[1], &val[0]);
ocp_status[2].IRQ1 = val[1];
ocp_status[2].IRQ0 = val[0];
//ocp_info("Cluster 2 BigOCPIntStatus : IRQ1 = %x, IRQ0 = %x\n", val[1], val[0]);
break;
*/
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
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_cluster0_enable);
seq_puts(m, "Cluster 0 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[0].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[0].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[0].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[0].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[0].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[0].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[0].CapTotAct);
/*
seq_printf(m, "CapMAFAct  = %d\n", ocp_status[0].CapMAFAct);
seq_printf(m, "CGAvgValid = %d\n", ocp_status[0].CGAvgValid);
seq_printf(m, "CGAvg      = %d %%\n", ocp_status[0].CGAvg);
seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[0].TopRawLkg);
seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[0].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[0].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[0].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[0].CPU3RawLkg);
*/
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
/*	 case 3:
LittleOCPIntStatus(0, &val[1], &val[0]);
ocp_status[0].IRQ1 = val[1];
ocp_status[0].IRQ0 = val[0];
//ocp_info("Cluster 0 LittleOCPIntStatus : IRQ1 = %x, IRQ0 = %x\n", val[1], val[0]);
break;
*/
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
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_cluster1_enable);
seq_puts(m, "Cluster 1 interrupt function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[1].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[1].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[1].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[1].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[1].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[1].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[1].CapTotAct);
/* seq_printf(m, "CapMAFAct  = %d\n", ocp_status[1].CapMAFAct);
seq_printf(m, "CGAvgValid = %d\n", ocp_status[1].CGAvgValid);
seq_printf(m, "CGAvg      = %d %%\n", ocp_status[1].CGAvg);
seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[1].TopRawLkg);
seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[1].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[1].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[1].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[1].CPU3RawLkg);
*/
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
/*	 case 3:
LittleOCPIntStatus(1, &val[1], &val[0]);
ocp_status[1].IRQ1 = val[1];
ocp_status[1].IRQ0 = val[0];
//ocp_info("Cluster 1 LittleOCPIntStatus : IRQ1 = %x, IRQ0 = %x\n", val[1], val[0]);
break;
*/
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
seq_printf(m, "Cluster 2 OCP Enable = %d\n", ocp_cluster2_enable);
seq_puts(m, "Cluster 2 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[2].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[2].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[2].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[2].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[2].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[2].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[2].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA(mW)\n", ocp_status[2].CapMAFAct);
seq_printf(m, "CGAvgValid   = %d\n", ocp_status[2].CGAvgValid);
seq_printf(m, "CGAvg        = %llu %%\n", ocp_status[2].CGAvg);
seq_printf(m, "TopRawLkg    = %d * 1.5uA\n", ocp_status[2].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d * 1.5uA\n", ocp_status[2].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d * 1.5uA\n", ocp_status[2].CPU1RawLkg);
/* seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[2].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[2].CPU3RawLkg);
*/
if (Reg_Debug_on) {
	int i;

	for (i = 0; i < 148; i += 4)
		seq_printf(m, "Addr: 0x%x = %x\n", (OCPAPBSTATUS00 + i), ocp_read(OCPAPBSTATUS00 + i));

	seq_printf(m, "Addr: 0x10206660 = %x\n", ocp_read(0x10206660));
	seq_printf(m, "Addr: 0x10206664 = %x\n", ocp_read(0x10206664));
	seq_printf(m, "Addr: 0x10206668 = %x\n", ocp_read(0x10206668));
	seq_printf(m, "Addr: 0x1020666C = %x\n", ocp_read(0x1020666C));
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

if (!((ocp_read_field(OCPAPBCFG00, 0:0) == 1) || (ocp_read_field(OCPAPBCFG00, 1:1) == 1))) {
	ocp_err("Please enable Cluster 2 OCPI or FPI.\n");
	free_page((unsigned long)buf);
	return count;
}


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
		ocp_status[2].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);
		/*
		ocp_info("Cluster 2 BigOCPCaptureStatus : Leakage = %d, Total = %d, ClkPct = %d (
		valid = %d)\n", Leakage, Total, ClkPct, ocp_read_field(OCPAPBSTATUS01, 0:0));
		*/
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
		/*
		ocp_info("Cluster 2 BigOCPCaptureStatus: CGAvg = %d (
		valid = %d)\n", CGAvg, ocp_read_field(OCPAPBSTATUS04, 27:27));
		*/
		break;
	case 7:
		BigOCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg);
		ocp_status[2].TopRawLkg = TopRawLkg;
		ocp_status[2].CPU0RawLkg = CPU0RawLkg;
		ocp_status[2].CPU1RawLkg = CPU1RawLkg;
		ocp_status[2].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);
		/*
		ocp_info("Cluster 2 BigOCPCaptureStatus: TopRawLkg = %d, CPU0RawLkg= %d, CPU1RawLkg = %d (
		valid = %d)\n", TopRawLkg, CPU0RawLkg, CPU1RawLkg, ocp_read_field(OCPAPBSTATUS01, 0:0));
		*/
		break;
	case 8:
			BigOCPMAFAct(&CapMAFAct);
			ocp_status[2].CapMAFAct = CapMAFAct;
			ocp_status[2].CaptureValid = ocp_read_field(OCPAPBSTATUS01, 0:0);
			break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;
		/*
		for (Count1 = 0; Count1 < 148 ; Count1 += 4)
		ocp_info("Address: 0x%x = %x\n", (OCPAPBSTATUS00 + Count1), ocp_read(OCPAPBSTATUS00 + Count1) );
		*/
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
seq_printf(m, "Cluster 0 OCP Enable = %d\n", ocp_cluster0_enable);
seq_puts(m, "Cluster 0 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[0].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[0].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[0].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[0].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[0].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[0].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[0].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA(mW)\n", ocp_status[0].CapMAFAct);
seq_printf(m, "AvgPwrValid  = %d\n", ocp_status[0].CGAvgValid);
seq_printf(m, "AvgAct       = %llu mA\n", ocp_status[0].CGAvg);
seq_printf(m, "AvgLkg       = %llu mA\n", ocp_status[0].AvgLkg);
seq_printf(m, "TopRawLkg    = %d * 1.5uA\n", ocp_status[0].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d * 1.5uA\n", ocp_status[0].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d * 1.5uA\n", ocp_status[0].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg   = %d * 1.5uA\n", ocp_status[0].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg   = %d * 1.5uA\n", ocp_status[0].CPU3RawLkg);

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

if (ocp_read_field(MP0_OCP_ENABLE, 6:0) != 0x7D) {
	ocp_err("Please enable Cluster 0 OCPI.\n");
	free_page((unsigned long)buf);
	return count;
}

if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) > 0) {
	switch (EnDis) {
	case 0:
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
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) == 4) {
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
		/*
		ocp_info("Cluster 0 LittleOCPCaptureGet : Leakage = %d, Total = %d, ClkPct = %d (
		valid = %d)\n", Leakage, Total, ClkPct, ocp_read_field(MP0_OCP_CAP_STATUS00,0:0));
		*/
		break;
	case 5:
			if (sscanf(buf, "%d %d %d", &EnDis, &Edge, &Count1) == 3) {
				if (LittleOCPAvgPwr(0, Edge, Count1) != 0)
					ocp_err("LittleOCPAvgPwr enable fail!");
			}
			break;
	case 6:
			LittleOCPAvgPwrGet(0, &AvgLkg, &AvgAct);
			ocp_status[0].CGAvgValid = ocp_read_field(MP0_OCP_DBG_STAT, 31:31);
			ocp_status[0].CGAvg = AvgAct;
			ocp_status[0].AvgLkg = AvgLkg;
				/*
				ocp_info("Cluster 0 LittleOCPAvgPwrGet: AvgAct = %llu AvgLkg = %llu
				(valid = %d)\n", AvgAct, AvgLkg, ocp_read_field(MP0_OCP_DBG_STAT, 31:31));
				*/
			break;
	case 7:
		CL0OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[0].TopRawLkg = TopRawLkg;
		ocp_status[0].CPU0RawLkg = CPU0RawLkg;
		ocp_status[0].CPU1RawLkg = CPU1RawLkg;
		ocp_status[0].CPU2RawLkg = CPU2RawLkg;
		ocp_status[0].CPU3RawLkg = CPU3RawLkg;
		ocp_status[0].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);
		/*
		ocp_info("Cluster 0: TopRawLkg=%d, CPU0RawLkg=%d, CPU1RawLkg=%d, CPU2RawLkg=%d, CPU3RawLkg=%d (
		valid = %d)\n",  TopRawLkg, CPU0RawLkg, CPU1RawLkg, CPU2RawLkg, CPU3RawLkg, ocp_read_field(
		MP0_OCP_CAP_STATUS00,0:0));
		*/
		break;
	case 8:
			LittleOCPMAFAct(0, &CapMAFAct);
			ocp_status[0].CapMAFAct = CapMAFAct;
			ocp_status[0].CaptureValid = ocp_read_field(MP0_OCP_CAP_STATUS00, 0:0);
			break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;

		/*
			for (Count1 = 0; Count1 < 524; Count1 += 4)
			ocp_info("Address: 0x%x = %x\n", (MP0_OCP_IRQSTATE + Count1), ocp_read(
			MP0_OCP_IRQSTATE + Count1));
		*/
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
seq_printf(m, "Cluster 1 OCP Enable = %d\n", ocp_cluster1_enable);
seq_puts(m, "Cluster 1 capture function:\n");
seq_printf(m, "IntEnDis     = 0x%x\n", ocp_status[1].IntEnDis);
seq_printf(m, "IRQ1         = 0x%x\n", ocp_status[1].IRQ1);
seq_printf(m, "IRQ0         = 0x%x\n", ocp_status[1].IRQ0);
seq_printf(m, "CapToLkg     = %d mA(mW)\n", ocp_status[1].CapToLkg);
seq_printf(m, "CapOCCGPct   = %d %%\n", ocp_status[1].CapOCCGPct);
seq_printf(m, "CaptureValid = %d\n", ocp_status[1].CaptureValid);
seq_printf(m, "CapTotAct    = %d mA(mW)\n", ocp_status[1].CapTotAct);
seq_printf(m, "CapMAFAct    = %d mA(mW)\n", ocp_status[1].CapMAFAct);
seq_printf(m, "AvgPwrValid  = %d\n", ocp_status[1].CGAvgValid);
seq_printf(m, "AvgAct       = %llu mA\n", ocp_status[1].CGAvg);
seq_printf(m, "AvgLkg       = %llu mA\n", ocp_status[1].AvgLkg);
seq_printf(m, "TopRawLkg    = %d * 1.5uA\n", ocp_status[1].TopRawLkg);
seq_printf(m, "CPU0RawLkg   = %d * 1.5uA\n", ocp_status[1].CPU0RawLkg);
seq_printf(m, "CPU1RawLkg   = %d * 1.5uA\n", ocp_status[1].CPU1RawLkg);
seq_printf(m, "CPU2RawLkg   = %d * 1.5uA\n", ocp_status[1].CPU2RawLkg);
seq_printf(m, "CPU3RawLkg   = %d * 1.5uA\n", ocp_status[1].CPU3RawLkg);

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

if (ocp_read_field(MP1_OCP_ENABLE, 6:0) != 0x7D) {
	ocp_err("Please enable Cluster 1 OCPI.\n");
	free_page((unsigned long)buf);
	return count;
}

if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) > 0) {
	switch (EnDis) {
	case 0:
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
		break;
	case 1:
		if (sscanf(buf, "%d %d %d %d", &EnDis, &Edge, &Count1, &Trig) == 4) {
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
		/*
		ocp_info("Cluster 1 LittleOCPCaptureGet : Leakage = %d, Total = %d, ClkPct = %d (
		valid = %d)\n", Leakage, Total, ClkPct, ocp_read_field(MP1_OCP_CAP_STATUS00,0:0));
		*/
		break;
	case 5:
			if (sscanf(buf, "%d %d %d", &EnDis, &Edge, &Count1) == 3) {
				if (LittleOCPAvgPwr(1, Edge, Count1) != 0)
					ocp_err("LittleOCPAvgPwr enable fail!");
			}
		break;
	case 6:
			LittleOCPAvgPwrGet(1, &AvgLkg, &AvgAct);
			ocp_status[1].CGAvgValid = ocp_read_field(MP1_OCP_DBG_STAT, 31:31);
			ocp_status[1].CGAvg = AvgAct;
			ocp_status[1].AvgLkg = AvgLkg;
			/*
			ocp_info("Cluster 1 LittleOCPAvgPwrGet: AvgAct = %llu  AvgLkg = %llu
			(valid = %d)\n", AvgAct, AvgLkg, ocp_read_field(MP1_OCP_DBG_STAT, 31:31));
			*/
			break;
	case 7:
		CL1OCPCaptureRawLkgStatus(&TopRawLkg, &CPU0RawLkg, &CPU1RawLkg, &CPU2RawLkg, &CPU3RawLkg);
		ocp_status[1].TopRawLkg = TopRawLkg;
		ocp_status[1].CPU0RawLkg = CPU0RawLkg;
		ocp_status[1].CPU1RawLkg = CPU1RawLkg;
		ocp_status[1].CPU2RawLkg = CPU2RawLkg;
		ocp_status[1].CPU3RawLkg = CPU3RawLkg;
		ocp_status[1].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);
		/*
		ocp_info("Cluster 1: TopRawLkg=%d, CPU0RawLkg=%d, CPU1RawLkg=%d, CPU2RawLkg=%d, CPU3RawLkg=%d (
		valid = %d)\n",  TopRawLkg, CPU0RawLkg, CPU1RawLkg, CPU2RawLkg, CPU3RawLkg, ocp_read_field(
		MP0_OCP_CAP_STATUS00,0:0));
		*/
		break;
	case 8:
			LittleOCPMAFAct(1, &CapMAFAct);
			ocp_status[1].CapMAFAct = CapMAFAct;
			ocp_status[1].CaptureValid = ocp_read_field(MP1_OCP_CAP_STATUS00, 0:0);
			break;
	case 9:
		if (sscanf(buf, "%d %d", &EnDis, &Count1) == 2)
			Reg_Debug_on = Count1;
		/*
			for (Count1 = 0; Count1 < 524; Count1 += 4)
			ocp_info("Address: 0x%x = %x\n", (MP1_OCP_IRQSTATE + Count1), ocp_read(
			MP1_OCP_IRQSTATE + Count1));
		*/
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
seq_printf(m, "Big_DREQ Enable = %d\n", big_dreq_enable);
seq_printf(m, "     BigDREQGet = %d\n", BigDREQGet());

seq_printf(m, "Little_DREQ Enable = %d\n", little_dreq_enable);
seq_printf(m, "     LittleDREQGet = %d\n", LittleDREQGet());

if (Reg_Debug_on) {
	seq_printf(m, "BIG_SRAMDREQ:    Addr: 0x%x = %x\n", BIG_SRAMDREQ, ocp_read(BIG_SRAMDREQ));
	seq_printf(m, "LITTLE_SRAMDREQ: Addr: 0x%x = %x\n", LITTLE_SRAMDREQ, ocp_read(LITTLE_SRAMDREQ));
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
		ret = BigDREQGet();
		/* ocp_info("Big DREQ = %d\n", ret);
			ocp_info("BIG_SRAMDREQ: Address: 0x%x = %x\n", BIG_SRAMDREQ, ocp_read(BIG_SRAMDREQ));
		*/
		break;
	case 3:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			ret = LittleDREQSWEn(val[0]);

			if (val[0] == 1 && ret == 0)
				little_dreq_enable = 1;
			else if (val[0] == 0 && ret == 0)
				little_dreq_enable = 0;
		}
		break;
	case 4:
		ret = LittleDREQGet();
		/* ocp_info("Little DREQ = %d\n", ret);
			ocp_info("LITTLE_SRAMDREQ: Address: 0x%x = %x\n", LITTLE_SRAMDREQ, ocp_read(LITTLE_SRAMDREQ));
		*/
		break;
	case 5:
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			ret = BigSRAMLDOEnable(val[0]);
			if (ret == 0)
				ocp_info("SRAM LDO volte = %dmv, enable success.\n", val[0]);
		/*
		ocp_info("BIG_SRAMLDO: Address: 0x%x = %x\n", BIG_SRAMLDO, ocp_read(BIG_SRAMLDO) );
		*/
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
	seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[2].CPU0RawLkg);
else if (dvt_test_on == 131 || dvt_test_on == 231)
	seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[2].CPU1RawLkg);
else if (dvt_test_on == 132 || dvt_test_on == 232)
	seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[2].TopRawLkg);
else if (dvt_test_on == 110 || dvt_test_on == 210)
	seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[0].CPU0RawLkg);
else if (dvt_test_on == 111 || dvt_test_on == 211)
	seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[0].CPU1RawLkg);
else if (dvt_test_on == 112 || dvt_test_on == 212)
	seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[0].CPU2RawLkg);
else if (dvt_test_on == 113 || dvt_test_on == 213)
	seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[0].CPU3RawLkg);
else if (dvt_test_on == 114 || dvt_test_on == 214)
	seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[0].TopRawLkg);
else if (dvt_test_on == 120 || dvt_test_on == 220)
	seq_printf(m, "CPU0RawLkg = %d * 1.5uA\n", ocp_status[1].CPU0RawLkg);
else if (dvt_test_on == 121 || dvt_test_on == 221)
	seq_printf(m, "CPU1RawLkg = %d * 1.5uA\n", ocp_status[1].CPU1RawLkg);
else if (dvt_test_on == 122 || dvt_test_on == 222)
	seq_printf(m, "CPU2RawLkg = %d * 1.5uA\n", ocp_status[1].CPU2RawLkg);
else if (dvt_test_on == 123 || dvt_test_on == 223)
	seq_printf(m, "CPU3RawLkg = %d * 1.5uA\n", ocp_status[1].CPU3RawLkg);
else if (dvt_test_on == 124 || dvt_test_on == 224)
	seq_printf(m, "TopRawLkg  = %d * 1.5uA\n", ocp_status[1].TopRawLkg);


return 0;
}

/* zzz */
static int set_cpu_opp(int oppidx_ll, int oppidx_l)
{
struct cpu_opp_info *opp_info_L = cpu_info_fy.cpu_opp_info_L;
struct cpu_opp_info *opp_info_LL = cpu_info_fy.cpu_opp_info_LL;


if ((oppidx_ll >= 8) || (oppidx_l >= 8))
	return 0;


ocp_write(0x1001A200, 0xF0000101);
ocp_write(0x1001A204, (opp_info_LL[oppidx_ll].pll | (opp_info_LL[oppidx_ll].postdiv << 24)));
/* LL bit 9:5, defautl use all 0  */
ocp_write_field(0x1001A274, 9:5, 0x0);

ocp_write(0x1001A210, 0xF0000101);
ocp_write(0x1001A214, (opp_info_L[oppidx_l].pll | (opp_info_L[oppidx_l].postdiv << 24)));
/* L bit 14:10, defautl use all 0 */
ocp_write_field(0x1001A274, 14:10, 0x0);

ocp_write(0x1001A274, (ocp_read(0x1001A274) & 0xffff83ff));

return 1;
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				LittleOCPConfig(300, 10000);
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
				ocp_write(0x10002890, ~(1<<6));
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
				ocp_write(0x10002890, ~(1<<6));
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
				ocp_write(0x10002890, ~(1<<6));
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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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

				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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

				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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

				ocp_write_field(0x10001FAC, 23:0, 0x0);
				/* 4. Enable AMUXOUT ball AGPIO (at SOC level config) */
				ocp_write(0x10002510, ~(1<<30));

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
		ocp_write(0x10005330, 0x331111);
		ocp_write(0x10005340, 0x1100333);
		break;
	case 5:
		/* PMUX SD CARD  */
		ocp_write(0x10005400, 0x04414440);
		break;
	case 6:
		/* AUXPMUX swithc */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3) {
			if (val[0] == 0) {
				if (val[1] == 0) {
					/* disable pinmux */
					ocp_write_field(0x10002510, 30:30, 0x1);
					/* L/LL CPU AGPIO prob out volt disable */
					ocp_write(0x10001fac, 0x000000ff);
				} else if (val[1] == 1) {
					/* enable pinmux */
					/* L/LL CPU AGPIO enable */
					ocp_write_field(0x10002510, 30:30, 0x0);
					/* L/LL CPU AGPIO prob out volt enable */
					ocp_write(0x10001fac, 0x0000ff00);
				}
			} else if (val[0] == 1) {
				if (val[1] == 0) {
					/* disable pinmux */
					/* L/LL CPU AGPIO prob out volt disable */
					ocp_write_field(0x10002890, 6:6, 0x1);
					ocp_write_field(0x102222b0, 31:31, 0x0);
				} else if (val[1] == 1) {
					/* enable pinmux */
					/* L/LL CPU AGPIO enable */
					ocp_write_field(0x10002890, 6:6, 0x0);
					ocp_write_field(0x102222b0, 31:31, 0x1);
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
			ocp_write(0x10001f98, 0x000000ff);
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

			ocp_write(0x10001f9c, wdata);
			ocp_write(0x10001fa0, wdata);
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
		/* LL, L , Big freq, opp idx */
		if (sscanf(buf, "%d %d %d", &function_id, &val[0], &val[1]) == 3)
			set_cpu_opp(val[0], val[1]);

		break;
	case 10:
		/* Big freq, opp idx  */
		if (sscanf(buf, "%d %d", &function_id, &val[0]) == 2) {
			if (val[0] < 8) {
				/* switch 26M */
				ocp_write_field(0x1001a270, 1:0, 0x00);
				udelay(100);
				BigiDVFSPllSetFreq(big_opp[val[0]]);
				udelay(100);
				/* switch armpll */
				ocp_write_field(0x1001a270, 1:0, 0x01);
			}
		}
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
	default:
		break;
	}
}
free_page((unsigned long)buf);
return count;
}






/* ocp_debug */
static int ocp_debug_proc_show(struct seq_file *m, void *v)
{
seq_printf(m, "OCP debug (log level) = %d\n", func_lv_mask);
return 0;
}

static ssize_t ocp_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
unsigned int dbg_lv;
char *buf = _copy_from_user_for_proc(buffer, count);

if (!buf)
	return -EINVAL;

if (kstrtoint(buf, 10, &dbg_lv) == 1)
	func_lv_mask = dbg_lv;
else
	ocp_err("echo dbg_lv (dec) > /proc/ocp/ocp_debug\n");

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

PROC_FOPS_RW(ocp_cluster2_enable);
PROC_FOPS_RW(ocp_cluster0_enable);
PROC_FOPS_RW(ocp_cluster1_enable);
PROC_FOPS_RW(ocp_cluster2_interrupt);
PROC_FOPS_RW(ocp_cluster0_interrupt);
PROC_FOPS_RW(ocp_cluster1_interrupt);
PROC_FOPS_RW(ocp_cluster2_capture);
PROC_FOPS_RW(ocp_cluster0_capture);
PROC_FOPS_RW(ocp_cluster1_capture);
PROC_FOPS_RW(ocp_debug);
PROC_FOPS_RW(dreq_function_test);
PROC_FOPS_RW(dvt_test);

static int _create_procfs(void)
{
struct proc_dir_entry *dir = NULL;
int i;

struct pentry {
const char *name;
const struct file_operations *fops;
};

const struct pentry entries[] = {
PROC_ENTRY(ocp_cluster2_enable),
PROC_ENTRY(ocp_cluster0_enable),
PROC_ENTRY(ocp_cluster1_enable),
PROC_ENTRY(ocp_cluster2_interrupt),
PROC_ENTRY(ocp_cluster0_interrupt),
PROC_ENTRY(ocp_cluster1_interrupt),
PROC_ENTRY(ocp_cluster2_capture),
PROC_ENTRY(ocp_cluster0_capture),
PROC_ENTRY(ocp_cluster1_capture),
PROC_ENTRY(ocp_debug),
PROC_ENTRY(dreq_function_test),
PROC_ENTRY(dvt_test),

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
if (node) {
	/* Setup IO addresses */
	ocp_base = of_iomap(node, 0);
}

/*get ocp irq num*/
ocp2_irq0_number = irq_of_parse_and_map(node, 0);
ocp2_irq1_number = irq_of_parse_and_map(node, 1);
ocp0_irq0_number = irq_of_parse_and_map(node, 2);
ocp0_irq1_number = irq_of_parse_and_map(node, 3);
ocp1_irq0_number = irq_of_parse_and_map(node, 4);
ocp1_irq1_number = irq_of_parse_and_map(node, 5);

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

/*
//here to turn on OCP
//set Enable=1
BigiDVFSEnable(2500, 110000, 120000); //idvfs enable 2500MHz, Vproc_x100mv, Vsram_x100mv
BigiDVFSChannel(1, 1);				  //ocp channel enable
BigOCPConfig(300, 10000);                 //cluster 2 Voffset=0.3v_x1000, Vstep=10mv_x1000000
BigOCPSetTarget(3, 127000);               //cluster 2 set OCPI/FPI, Target=127 W
BigOCPEnable(3,1,625,0);                  //cluster 2 set OCPI/FPI, Target_unit=mW, CG=6.25%_x100
LittleOCPConfig(300, 10000);              //cluster 0/1 Voffset=0.5v_x1000, Vstep=6.25mv_x1000000
LittleOCPSetTarget(0, 127000);            //cluster 0 Target=127W_x1000
LittleOCPDVFSSet(0, 1000, 1000);          //cluster 0 FreqMHz, VoltInmV
LittleOCPSetTarget(1, 127000);            //cluster 1 Target=127W_x1000
LittleOCPDVFSSet(1, 1000, 1000);	      //cluster 0 FreqMHz, VoltInmV
LittleOCPEnable(0, 1, 625);               //cluster 0 Target_unit=mW, CG=6.25_x100
LittleOCPEnable(1, 1, 625);               //cluster 1 Target_unit=mW, CG=6.25_x100
ocp_cluster0_enable = 1;
ocp_cluster1_enable = 1;
ocp_cluster2_enable = 1;
*/

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
