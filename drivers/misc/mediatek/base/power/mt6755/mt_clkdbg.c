#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk-provider.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/version.h>

#define DUMMY_REG_TEST		0
#define DUMP_INIT_STATE		0
#define CLKDBG_PM_DOMAIN	0
#define CLKDBG_6755		1
#define CLKDBG_6755_TK		0
#define CLKDBG_8173		0

#define TAG	"[clkdbg] "

#define clk_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

/************************************************
 **********      register access       **********
 ************************************************/

#ifndef BIT
#define BIT(_bit_)		(u32)(1U << (_bit_))
#endif

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_setl(addr, val)	clk_writel(addr, clk_readl(addr) | (val))
#define clk_clrl(addr, val)	clk_writel(addr, clk_readl(addr) & ~(val))
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

/************************************************
 **********      struct definition     **********
 ************************************************/

#if CLKDBG_6755

static void __iomem *topckgen_base;	/* 0x10000000 */
static void __iomem *infrasys_base;	/* 0x10001000 */
static void __iomem *perisys_base;	/* 0x10003000 */
static void __iomem *mcucfg_base;	/* 0x10200000 */
static void __iomem *apmixedsys_base;	/* 0x1000C000 */
static void __iomem *audiosys_base;	/* 0x11220000 */
static void __iomem *mfgsys_base;	/* 0x13000000 */
static void __iomem *mmsys_base;	/* 0x14000000 */
static void __iomem *imgsys_base;	/* 0x15000000 */
static void __iomem *vdecsys_base;	/* 0x16000000 */
static void __iomem *vencsys_base;	/* 0x17000000 */
static void __iomem *scpsys_base;	/* 0x10006000 */

#define TOPCKGEN_REG(ofs)	(topckgen_base + ofs)
#define INFRA_REG(ofs)		(infrasys_base + ofs)
#define PREI_REG(ofs)		(perisys_base + ofs)
#define MCUCFG_REG(ofs)		(mcucfg_base + ofs)
#define APMIXED_REG(ofs)	(apmixedsys_base + ofs)
#define AUDIO_REG(ofs)		(audiosys_base + ofs)
#define MFG_REG(ofs)		(mfgsys_base + ofs)
#define MM_REG(ofs)		(mmsys_base + ofs)
#define IMG_REG(ofs)		(imgsys_base + ofs)
#define VDEC_REG(ofs)		(vdecsys_base + ofs)
#define VENC_REG(ofs)		(vencsys_base + ofs)
#define SCP_REG(ofs)		(scpsys_base + ofs)

#define CLK_MISC_CFG_0		TOPCKGEN_REG(0x104)
#define CLK_MISC_CFG_1		TOPCKGEN_REG(0x108)
#define CLK_DBG_CFG		TOPCKGEN_REG(0x10C)
#define CLK26CALI_0		TOPCKGEN_REG(0x220)
#define CLK26CALI_1		TOPCKGEN_REG(0x224)

#define CLK_CFG_0		TOPCKGEN_REG(0x040)
#define CLK_CFG_1		TOPCKGEN_REG(0x050)
#define CLK_CFG_2		TOPCKGEN_REG(0x060)
#define CLK_CFG_3		TOPCKGEN_REG(0x070)
#define CLK_CFG_4		TOPCKGEN_REG(0x080)
#define CLK_CFG_5		TOPCKGEN_REG(0x090)
#define CLK_CFG_6		TOPCKGEN_REG(0x0A0)
#define CLK_CFG_7		TOPCKGEN_REG(0x0B0)
#define CLK_CFG_8		TOPCKGEN_REG(0x0C0)
#define CLK_CFG_9		TOPCKGEN_REG(0x0D0)


/* APMIXEDSYS Register */

#define ARMCA15PLL_CON0		APMIXED_REG(0x200)
#define ARMCA15PLL_CON1		APMIXED_REG(0x204)
#define ARMCA15PLL_PWR_CON0	APMIXED_REG(0x20C)

#define ARMCA7PLL_CON0		APMIXED_REG(0x210)
#define ARMCA7PLL_CON1		APMIXED_REG(0x214)
#define ARMCA7PLL_PWR_CON0	APMIXED_REG(0x21C)

#define MAINPLL_CON0		APMIXED_REG(0x220)
#define MAINPLL_CON1		APMIXED_REG(0x224)
#define MAINPLL_PWR_CON0	APMIXED_REG(0x22C)

#define UNIVPLL_CON0		APMIXED_REG(0x230)
#define UNIVPLL_CON1		APMIXED_REG(0x234)
#define UNIVPLL_PWR_CON0	APMIXED_REG(0x23C)

#define MMPLL_CON0		APMIXED_REG(0x240)
#define MMPLL_CON1		APMIXED_REG(0x244)
#define MMPLL_PWR_CON0		APMIXED_REG(0x24C)

#define MSDCPLL_CON0		APMIXED_REG(0x250)
#define MSDCPLL_CON1		APMIXED_REG(0x254)
#define MSDCPLL_PWR_CON0	APMIXED_REG(0x25C)

#define VENCPLL_CON0		APMIXED_REG(0x260)
#define VENCPLL_CON1		APMIXED_REG(0x264)
#define VENCPLL_PWR_CON0	APMIXED_REG(0x26C)

#define TVDPLL_CON0		APMIXED_REG(0x270)
#define TVDPLL_CON1		APMIXED_REG(0x274)
#define TVDPLL_PWR_CON0		APMIXED_REG(0x27C)

#define MPLL_CON0		APMIXED_REG(0x280)
#define MPLL_CON1		APMIXED_REG(0x284)
#define MPLL_PWR_CON0		APMIXED_REG(0x28C)

#define VCODECPLL_CON0		APMIXED_REG(0x290)
#define VCODECPLL_CON1		APMIXED_REG(0x294)
#define VCODECPLL_PWR_CON0	APMIXED_REG(0x29C)

#define APLL1_CON0		APMIXED_REG(0x2A0)
#define APLL1_CON1		APMIXED_REG(0x2A4)
#define APLL1_PWR_CON0		APMIXED_REG(0x2B0)

#define APLL2_CON0		APMIXED_REG(0x2B4)
#define APLL2_CON1		APMIXED_REG(0x2B8)
#define APLL2_PWR_CON0		APMIXED_REG(0x2C4)

/* INFRASYS Register */

#define INFRA_TOPCKGEN_DCMCTL	INFRA_REG(0x0010)
/*
#define INFRA_PDN_SET		INFRA_REG(0x0040)
#define INFRA_PDN_CLR		INFRA_REG(0x0044)
#define INFRA_PDN_STA		INFRA_REG(0x0048)
*/
#define INFRA_PDN0_SET		INFRA_REG(0x0080)
#define INFRA_PDN0_CLR		INFRA_REG(0x0084)
#define INFRA_PDN0_STA		INFRA_REG(0x0090)

#define INFRA_PDN1_SET		INFRA_REG(0x0088)
#define INFRA_PDN1_CLR		INFRA_REG(0x008C)
#define INFRA_PDN1_STA		INFRA_REG(0x0094)

#define INFRA_PDN2_SET		INFRA_REG(0x00A4)
#define INFRA_PDN2_CLR		INFRA_REG(0x00A8)
#define INFRA_PDN2_STA		INFRA_REG(0x00AC)

/* Audio Register */

#define AUDIO_TOP_CON0		AUDIO_REG(0x0000)

/* MFGCFG Register */

#define MFG_CG_CON		MFG_REG(0)
#define MFG_CG_SET		MFG_REG(4)
#define MFG_CG_CLR		MFG_REG(8)

/* MMSYS Register */

#define DISP_CG_CON0		MM_REG(0x100)
#define DISP_CG_SET0		MM_REG(0x104)
#define DISP_CG_CLR0		MM_REG(0x108)
#define DISP_CG_CON1		MM_REG(0x110)
#define DISP_CG_SET1		MM_REG(0x114)
#define DISP_CG_CLR1		MM_REG(0x118)

/* IMGSYS Register */
#define IMG_CG_CON		IMG_REG(0x0000)
#define IMG_CG_SET		IMG_REG(0x0004)
#define IMG_CG_CLR		IMG_REG(0x0008)

/* VDEC Register */
#define VDEC_CKEN_SET		VDEC_REG(0x0000)
#define VDEC_CKEN_CLR		VDEC_REG(0x0004)
#define LARB_CKEN_SET		VDEC_REG(0x0008)/*LARB1*/
#define LARB_CKEN_CLR		VDEC_REG(0x000C)/*LARB1*/

/* VENC Register */
#define VENC_CG_CON		VENC_REG(0x0)
#define VENC_CG_SET		VENC_REG(0x4)
#define VENC_CG_CLR		VENC_REG(0x8)

/* SCPSYS Register */
#define SPM_VDE_PWR_CON		SCP_REG(0x300)
#define SPM_MFG_PWR_CON		SCP_REG(0x338)
#define SPM_VEN_PWR_CON		SCP_REG(0x304)
#define SPM_ISP_PWR_CON		SCP_REG(0x308)
#define SPM_DIS_PWR_CON		SCP_REG(0x30C)
#define SPM_AUDIO_PWR_CON	SCP_REG(0x314)
#define SPM_MFG_ASYNC_PWR_CON	SCP_REG(0x334)
#define SPM_PWR_STATUS		SCP_REG(0x180)
#define SPM_PWR_STATUS_2ND	SCP_REG(0x184)

#endif /* CLKDBG_SOC */

static bool is_valid_reg(void __iomem *addr)
{
	#ifndef CONFIG_ARM64
		return 1;
	#else
		return ((u64)addr & 0xf0000000) || (((u64)addr >> 32) & 0xf0000000);
	#endif
}

enum FMETER_TYPE {
	ABIST,
	CKGEN
};

enum ABIST_CLK {
	ABIST_CLK_NULL,

#if CLKDBG_6755
	AD_CSI0_DELAY_TSTCLK	= 1,
	AD_CSI1_DELAY_TSTCLK	= 2,
	AD_PSMCUPLL_CK	= 3,
	AD_L1MCUPLL_CK		= 4,
	AD_C2KCPPLL_CK		= 5,
	AD_ICCPLL_CK		= 6,
	AD_INTFPLL_CK		= 7,
	AD_MD2GPLL_CK		= 8,
	AD_IMCPLL_CK			= 9,
	AD_C2KDSPPLL_CK			= 10,
	AD_BRPPLL_CK			= 11,
	AD_DFEPLL_CK		= 12,
	AD_EQPLL_CK		= 13,
	AD_CMPPLL_CK		= 14,
	AD_MDPLLGP_TST_CK		= 15,
	AD_MDPLL1_FS26M_CK			= 16,
	AD_MDPLL1_FS208M_CK		= 17,
	AD_MDPLL1_FS393P216M_CK		= 18,
	AD_ARMBPLL_1700M_PRO_CK		= 19,
	AD_ARMBPLL_1700M_CORE_CK		= 20,
	AD_ARMSPLL_1000M_PRO_CK		= 21,
	AD_ARMSPLL_1000M_CORE_CK		= 22,
	AD_MAINPLL_1092M_CK			= 23,
	AD_UNIVPLL_1248M_CK			= 24,
	AD_MMPLL_520M_CK			= 25,
	AD_MSDCPLL_416M_CK			= 26,
	AD_VENCPLL_286M_CK		= 27,
	AD_APLL1_180P6336M_CK		= 28,
	AD_APLL2_196P608M_CK		= 29,
	AD_APPLLGP_TST_CK		= 30,
	AD_USB20_48M_CK			= 31,
	AD_UNIV_48M_CK			= 32,
	AD_SSUSB_48M_CK	= 33,
	AD_TVDPLL_594M_CK	= 34,
	AD_DSI0_MPPLL_TST_CK			= 35,
	AD_DSI0_LNTC_DSICLK		= 36,
	AD_MEMPLL_MONCLK			= 37,
	AD_OSC_CK		= 38,
	RTC32K_CK_I	= 39,
	ARMPLL_HD_FARM_CK_BIG			= 40,
	ARMPLL_HD_FARM_CK_SML		= 41,
	ARMPLL_HD_FARM_CK_BUS	= 42,
	MSDC01_IN_CK			= 43,
	MSDC02_IN_CK			= 44,
	MSDC11_IN_CK			= 45,
	MSDC12_IN_CK			= 46,
	AD_DSI0_DPICLK		= 47,
#endif /* CLKDBG_6755 */

	ABIST_CLK_END,
};

static const char * const ABIST_CLK_NAME[] = {
#if CLKDBG_6755
	[AD_CSI0_DELAY_TSTCLK]	= "AD_CSI0_DELAY_TSTCLK",
	[AD_CSI1_DELAY_TSTCLK]	= "AD_CSI1_DELAY_TSTCLK",
	[AD_PSMCUPLL_CK]	= "AD_PSMCUPLL_CK",
	[AD_L1MCUPLL_CK]		= "AD_L1MCUPLL_CK",
	[AD_C2KCPPLL_CK]		= "AD_C2KCPPLL_CK",
	[AD_ICCPLL_CK]		= "AD_ICCPLL_CK",
	[AD_INTFPLL_CK]		= "AD_INTFPLL_CK",
	[AD_MD2GPLL_CK]		= "AD_MD2GPLL_CK",
	[AD_IMCPLL_CK]		= "AD_IMCPLL_CK",
	[AD_C2KDSPPLL_CK]		= "AD_C2KDSPPLL_CK",
	[AD_BRPPLL_CK]		= "AD_BRPPLL_CK",
	[AD_DFEPLL_CK]		= "AD_DFEPLL_CK",
	[AD_EQPLL_CK]		= "AD_EQPLL_CK",
	[AD_CMPPLL_CK]		= "AD_CMPPLL_CK",
	[AD_MDPLLGP_TST_CK]		= "AD_MDPLLGP_TST_CK",
	[AD_MDPLL1_FS26M_CK]			= "AD_MDPLL1_FS26M_CK",
	[AD_MDPLL1_FS208M_CK]		= "AD_MDPLL1_FS208M_CK",
	[AD_MDPLL1_FS393P216M_CK]		= "AD_MDPLL1_FS393P216M_CK",
	[AD_ARMBPLL_1700M_PRO_CK]		= "AD_ARMBPLL_1700M_PRO_CK",
	[AD_ARMBPLL_1700M_CORE_CK]		= "AD_ARMBPLL_1700M_CORE_CK",
	[AD_ARMSPLL_1000M_PRO_CK]	= "AD_ARMSPLL_1000M_PRO_CK",
	[AD_ARMSPLL_1000M_CORE_CK]		= "AD_ARMSPLL_1000M_CORE_CK",
	[AD_MAINPLL_1092M_CK]			= "AD_MAINPLL_1092M_CK",
	[AD_UNIVPLL_1248M_CK]			= "AD_UNIVPLL_1248M_CK",
	[AD_MMPLL_520M_CK]			= "AD_MMPLL_520M_CK",
	[AD_MSDCPLL_416M_CK]		= "AD_MSDCPLL_416M_CK",
	[AD_VENCPLL_286M_CK]	= "AD_VENCPLL_286M_CK",
	[AD_APLL1_180P6336M_CK]		= "AD_APLL1_180P6336M_CK",
	[AD_APLL2_196P608M_CK]			= "AD_APLL2_196P608M_CK",
	[AD_APPLLGP_TST_CK]			= "AD_APPLLGP_TST_CK",
	[AD_USB20_48M_CK]	= "AD_USB20_48M_CK",
	[AD_UNIV_48M_CK]	= "AD_UNIV_48M_CK",
	[AD_SSUSB_48M_CK]		= "AD_SSUSB_48M_CK",
	[AD_TVDPLL_594M_CK]	= "AD_TVDPLL_594M_CK",
	[AD_DSI0_MPPLL_TST_CK]		= "AD_DSI0_MPPLL_TST_CK",
	[AD_DSI0_LNTC_DSICLK]		= "AD_DSI0_LNTC_DSICLK",
	[AD_MEMPLL_MONCLK]	= "AD_MEMPLL_MONCLK",
	[AD_OSC_CK]		= "AD_OSC_CK",
	[RTC32K_CK_I]	= "rtc32k_ck_i",
	[ARMPLL_HD_FARM_CK_BIG]	= "armpll_hd_farm_ck_big",
	[ARMPLL_HD_FARM_CK_SML]		= "armpll_hd_farm_ck_sml",
	[ARMPLL_HD_FARM_CK_BUS]		= "armpll_hd_farm_ck_bus",
	[MSDC01_IN_CK]			= "msdc01_in_ck",
	[MSDC02_IN_CK]		= "msdc02_in_ck",
	[MSDC11_IN_CK]		= "msdc11_in_ck",
	[MSDC12_IN_CK]		= "msdc12_in_ck",
	[AD_DSI0_DPICLK]		= "AD_DSI0_DPICLK",
#endif /* CLKDBG_6755 */
};

enum CKGEN_CLK {
	CKGEN_CLK_NULL,

#if CLKDBG_6755
	HD_FAXI_CK			= 1,
	HF_FDDRPHYCFG_CK			= 2,
	HF_FMM_CK			= 3,
	F_FPWM_CK		= 4,
	HF_FVDEC_CK			= 5,
	HF_FMFG_CK			= 6,
	HF_FCAMTG_CK			= 7,
	F_FUART_CK			= 8,
	HF_FSPI_CK			= 9,
	HF_FMSDC50_0_HCLK_CK			= 10,
	HF_FMSDC50_0_CK			= 11,
	HF_FMSDC30_1_CK			= 12,
	HF_FMSDC30_2_CK			= 13,
	HF_FMSDC30_3_CK			= 14,
	HF_FAUDIO_CK		= 15,
	HF_FAUD_INTBUS_CK			= 16,
	HF_FPMICSPI_CK			= 17,
	HF_FATB_CK			= 19,
	HF_FMJC_CK			= 20,
	HF_FDPI0_CK		= 21,
	HF_FSCAM_CK			= 22,
	HF_FAUD_1_CK			= 23,
	HF_FAUD_2_CK			= 24,
	ECO_AO12_MEM_MUX_CK			= 25,
	ECO_AO12_MEM_DCM_CK			= 26,
	F_FDISP_PWM_CK			= 27,
	F_FSSUSB_TOP_SYS_CK			= 28,
	F_FSSUSB_TOP_XHCI_CK			= 29,
	F_FUSB_TOP_CK		= 30,
	HG_FSPM_CK		= 31,
	HF_FBSI_SPI_CK			= 32,
	F_FI2C_CK			= 33,
	HG_FDVFSP_CK			= 34,
#endif /* CLKDBG_6755 */

	CKGEN_CLK_END,
};

static const char * const CKGEN_CLK_NAME[] = {
#if CLKDBG_6755
	[HD_FAXI_CK]			= "hd_faxi_ck",
	[HF_FDDRPHYCFG_CK]			= "hf_fddrphycfg_ck",
	[HF_FMM_CK]			= "hf_fmm_ck",
	[F_FPWM_CK]		= "f_fpwm_ck",
	[HF_FVDEC_CK]			= "hf_fvdec_ck",
	[HF_FMFG_CK]			= "hf_fmfg_ck",
	[HF_FCAMTG_CK]			= "hf_fcamtg_ck",
	[F_FUART_CK]			= "f_fuart_ck",
	[HF_FSPI_CK]			= "hf_fspi_ck",
	[HF_FMSDC50_0_HCLK_CK]			= "hf_fmsdc50_0_h_ck",
	[HF_FMSDC50_0_CK]			= "hf_fmsdc50_0_ck",
	[HF_FMSDC30_1_CK]			= "hf_fmsdc30_1_ck",
	[HF_FMSDC30_2_CK]			= "hf_fmsdc30_2_ck",
	[HF_FMSDC30_3_CK]			= "hf_fmsdc30_3_ck",
	[HF_FAUDIO_CK]		= "hf_faudio_ck",
	[HF_FAUD_INTBUS_CK]		= "hf_faud_intbus_ck",
	[HF_FPMICSPI_CK]		= "hf_fpmicspi_ck",
	[HF_FATB_CK]		= "hf_fatb_ck",
	[HF_FMJC_CK]		= "hf_fmjc_ck",
	[HF_FDPI0_CK]			= "hf_fdpi0_ck",
	[HF_FSCAM_CK]		= "hf_fscam_ck",
	[HF_FAUD_1_CK]		= "hf_faud_1_ck",
	[HF_FAUD_2_CK]			= "hf_faud_2_ck",
	[ECO_AO12_MEM_MUX_CK]			= "eco_ao12_mem_mux_ck",
	[ECO_AO12_MEM_DCM_CK]		= "eco_ao12_mem_dcm_ck",
	[F_FDISP_PWM_CK]			= "f_fdisp_pwm_ck",
	[F_FSSUSB_TOP_SYS_CK]			= "f_fssusb_top_sys_ck",
	[F_FSSUSB_TOP_XHCI_CK]			= "f_fssusb_top_xhci_ck",
	[F_FUSB_TOP_CK]			= "f_fusb_top_ck",
	[HG_FSPM_CK]		= "hg_fspm_ck",
	[HF_FBSI_SPI_CK]		= "hf_fbsi_spi_ck",
	[F_FI2C_CK]			= "f_fi2c_ck",
	[HG_FDVFSP_CK]			= "hg_fdvfsp_ck",
#endif /* CLKDBG_6755 */
};

#if CLKDBG_8173

static void set_fmeter_divider_ca7(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 15, 8, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

static void set_fmeter_divider_ca15(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_2);

	v = ALT_BITS(v, 7, 0, k1);
	clk_writel(CLK_MISC_CFG_2, v);
}
#endif /* CLKDBG_6755 */


#if CLKDBG_6755
static void set_fmeter_divider(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_0);

	v = ALT_BITS(v, 31, 24, k1);
	clk_writel(CLK_MISC_CFG_0, v);
}

static bool wait_fmeter_done(u32 tri_bit)
{
	static int max_wait_count;
	int wait_count = (max_wait_count > 0) ? (max_wait_count * 2 + 2) : 100;
	int i;

	/* wait fmeter */
	for (i = 0; i < wait_count && (clk_readl(CLK26CALI_0) & tri_bit); i++)
		udelay(20);

	if (!(clk_readl(CLK26CALI_0) & tri_bit)) {
		max_wait_count = max(max_wait_count, i);
		return true;
	}

	return false;
}

#endif /* CLKDBG_6755 */

static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	void __iomem *clk_cfg_reg = CLK_DBG_CFG;
	void __iomem *cnt_reg =	CLK26CALI_1;
	u32 cksw_ckgen[] = {13, 8, clk};
	u32 cksw_abist[] = {21, 16, clk};
	u32 *cksw_hlv =		(type == CKGEN) ? cksw_ckgen	: cksw_abist;
	u32 en_bit =	BIT(12);
	u32 tri_bit =	BIT(4);
	u32 clk_misc_cfg_0;
	u32 clk_misc_cfg_1;
	u32 clk_cfg_val;
	u32 freq = 0;

	if (!is_valid_reg(topckgen_base))
		return 0;

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);	/* backup reg value */
	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);
	clk_cfg_val = clk_readl(clk_cfg_reg);
	/* set meter div (0 = /1) */
	set_fmeter_divider(k1);
	/* select fmeter */
	if (type == CKGEN)
		clk_setl(CLK_DBG_CFG, BIT(0));
	else
		clk_clrl(CLK_DBG_CFG, BIT(1));
	/* select meter clock input */
	clk_writel_mask(CLK_DBG_CFG, cksw_hlv[0], cksw_hlv[1], cksw_hlv[2]);
	/* trigger fmeter */
	clk_setl(CLK26CALI_0, en_bit);			/* enable fmeter_en */
	clk_clrl(CLK26CALI_0, tri_bit);			/* start fmeter */
	clk_setl(CLK26CALI_0, en_bit|tri_bit);

	if (wait_fmeter_done(tri_bit)) {
		u32 cnt = clk_readl(cnt_reg) & 0xFFFF;

		freq = (cnt * 26000) / 1024;
	}
	/* restore register settings */
	clk_writel(clk_cfg_reg, clk_cfg_val);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);
	return freq;
}

static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 last_freq = 0;
	u32 freq;
	u32 maxfreq;

	freq = fmeter_freq(type, k1, clk);
	maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, k1, clk);
		maxfreq = max(freq, last_freq);
	}

	return freq;
}

static u32 measure_abist_freq(enum ABIST_CLK clk)
{
	return measure_stable_fmeter_freq(ABIST, 0, clk);
}

static u32 measure_ckgen_freq(enum CKGEN_CLK clk)
{
	return measure_stable_fmeter_freq(CKGEN, 0, clk);
}



static enum ABIST_CLK abist_clk[] = {
	AD_CSI0_DELAY_TSTCLK,
	AD_CSI1_DELAY_TSTCLK,
	AD_PSMCUPLL_CK,
	AD_L1MCUPLL_CK,
	AD_C2KCPPLL_CK,
	AD_ICCPLL_CK,
	AD_INTFPLL_CK,
	AD_MD2GPLL_CK,
	AD_IMCPLL_CK,
	AD_C2KDSPPLL_CK,
	AD_BRPPLL_CK,
	AD_DFEPLL_CK,
	AD_EQPLL_CK,
	AD_CMPPLL_CK,
	AD_MDPLLGP_TST_CK,
	AD_MDPLL1_FS26M_CK,
	AD_MDPLL1_FS208M_CK,
	AD_MDPLL1_FS393P216M_CK,
	AD_ARMBPLL_1700M_PRO_CK,
	AD_ARMBPLL_1700M_CORE_CK,
	AD_ARMSPLL_1000M_PRO_CK,
	AD_ARMSPLL_1000M_CORE_CK,
	AD_MAINPLL_1092M_CK,
	AD_UNIVPLL_1248M_CK,
	AD_MMPLL_520M_CK,
	AD_MSDCPLL_416M_CK,
	AD_VENCPLL_286M_CK,
	AD_APLL1_180P6336M_CK,
	AD_APLL2_196P608M_CK,
	AD_APPLLGP_TST_CK,
	AD_USB20_48M_CK,
	AD_UNIV_48M_CK,
	AD_SSUSB_48M_CK,
	AD_TVDPLL_594M_CK,
	AD_DSI0_MPPLL_TST_CK,
	AD_DSI0_LNTC_DSICLK,
	AD_MEMPLL_MONCLK,
	AD_OSC_CK,
	RTC32K_CK_I,
	ARMPLL_HD_FARM_CK_BIG,
	ARMPLL_HD_FARM_CK_SML,
	ARMPLL_HD_FARM_CK_BUS,
	MSDC01_IN_CK,
	MSDC02_IN_CK,
	MSDC11_IN_CK,
	MSDC12_IN_CK,
	AD_DSI0_DPICLK,
};

static enum CKGEN_CLK ckgen_clk[] = {
	HD_FAXI_CK,
	HF_FDDRPHYCFG_CK,
	HF_FMM_CK,
	F_FPWM_CK,
	HF_FVDEC_CK,
	HF_FMFG_CK,
	HF_FCAMTG_CK,
	F_FUART_CK,
	HF_FSPI_CK,
	HF_FMSDC50_0_HCLK_CK,
	HF_FMSDC50_0_CK,
	HF_FMSDC30_1_CK,
	HF_FMSDC30_2_CK,
	HF_FMSDC30_3_CK,
	HF_FAUDIO_CK,
	HF_FAUD_INTBUS_CK,
	HF_FPMICSPI_CK,
	HF_FATB_CK,
	HF_FMJC_CK,
	HF_FDPI0_CK,
	HF_FSCAM_CK,
	HF_FAUD_1_CK,
	HF_FAUD_2_CK,
	ECO_AO12_MEM_MUX_CK,
	ECO_AO12_MEM_DCM_CK,
	F_FDISP_PWM_CK,
	F_FSSUSB_TOP_SYS_CK,
	F_FSSUSB_TOP_XHCI_CK,
	F_FUSB_TOP_CK,
	HG_FSPM_CK,
	HF_FBSI_SPI_CK,
	F_FI2C_CK,
	HG_FDVFSP_CK,
};
#if CLKDBG_8173
static u32 measure_armpll_freq(u32 jit_ctrl)
{
#if CLKDBG_8173
	u32 freq;
	u32 mcu26c;
	u32 armpll_jit_ctrl;
	u32 top_dcmctl;

	if (!is_valid_reg(mcucfg_base) || !is_valid_reg(infrasys_base))
		return 0;

	mcu26c = clk_readl(MCU_26C);
	armpll_jit_ctrl = clk_readl(ARMPLL_JIT_CTRL);
	top_dcmctl = clk_readl(INFRA_TOPCKGEN_DCMCTL);

	clk_setl(MCU_26C, 0x8);
	clk_setl(ARMPLL_JIT_CTRL, jit_ctrl);
	clk_clrl(INFRA_TOPCKGEN_DCMCTL, 0x700);

	freq = measure_stable_fmeter_freq(ABIST, 1, ARMPLL_OCC_MON);

	clk_writel(INFRA_TOPCKGEN_DCMCTL, top_dcmctl);
	clk_writel(ARMPLL_JIT_CTRL, armpll_jit_ctrl);
	clk_writel(MCU_26C, mcu26c);

	return freq;
#else
	return 0;
#endif
}

static u32 measure_ca53_freq(void)
{
	return measure_armpll_freq(0x01);
}

static u32 measure_ca57_freq(void)
{
	return measure_armpll_freq(0x11);
}

#endif /* CLKDBG_8173 */

#if DUMP_INIT_STATE

static void print_abist_clock(enum ABIST_CLK clk)
{
	u32 freq = measure_abist_freq(clk);

	clk_info("%2d: %-29s: %u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void print_ckgen_clock(enum CKGEN_CLK clk)
{
	u32 freq = measure_ckgen_freq(clk);

	clk_info("%2d: %-29s: %u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

static void print_fmeter_all(void)
{
	size_t i;
	u32 freq;

	if (!is_valid_reg(apmixedsys_base))
		return;

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		print_abist_clock(abist_clk[i]);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		print_ckgen_clock(ckgen_clk[i]);
}

#endif /* DUMP_INIT_STATE */

static void seq_print_abist_clock(enum ABIST_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_abist_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void seq_print_ckgen_clock(enum CKGEN_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_ckgen_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

static int seq_print_fmeter_all(struct seq_file *s, void *v)
{
	size_t i;

	if (!is_valid_reg(apmixedsys_base))
		return 0;

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		seq_print_abist_clock(abist_clk[i], s, v);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		seq_print_ckgen_clock(ckgen_clk[i], s, v);

	return 0;
}

struct regname {
	void __iomem *reg;
	const char *name;
};

static size_t get_regnames(struct regname *regnames, size_t size)
{
	struct regname rn[] = {
#if CLKDBG_6755
		{SPM_VDE_PWR_CON, "SPM_VDE_PWR_CON"},
		{SPM_MFG_PWR_CON, "SPM_MFG_PWR_CON"},
		{SPM_VEN_PWR_CON, "SPM_VEN_PWR_CON"},
		{SPM_ISP_PWR_CON, "SPM_ISP_PWR_CON"},
		{SPM_DIS_PWR_CON, "SPM_DIS_PWR_CON"},
		{SPM_AUDIO_PWR_CON, "SPM_AUDIO_PWR_CON"},
		{SPM_MFG_ASYNC_PWR_CON, "SPM_MFG_ASYNC_PWR_CON"},
		{SPM_PWR_STATUS, "SPM_PWR_STATUS"},
		{SPM_PWR_STATUS_2ND, "SPM_PWR_STATUS_2ND"},
		{CLK_CFG_0, "CLK_CFG_0"},
		{CLK_CFG_1, "CLK_CFG_1"},
		{CLK_CFG_2, "CLK_CFG_2"},
		{CLK_CFG_3, "CLK_CFG_3"},
		{CLK_CFG_4, "CLK_CFG_4"},
		{CLK_CFG_5, "CLK_CFG_5"},
		{CLK_CFG_6, "CLK_CFG_6"},
		{CLK_CFG_7, "CLK_CFG_7"},
		{CLK_CFG_8, "CLK_CFG_8"},
		{CLK_CFG_9, "CLK_CFG_9"},
		{CLK_MISC_CFG_0, "CLK_MISC_CFG_0"},
		{CLK_MISC_CFG_1, "CLK_MISC_CFG_1"},
		{CLK26CALI_0, "CLK26CALI_0"},
		{CLK26CALI_1, "CLK26CALI_1"},
		{ARMCA15PLL_CON0, "ARMCA15PLL_CON0"},
		{ARMCA15PLL_CON1, "ARMCA15PLL_CON1"},
		{ARMCA15PLL_PWR_CON0, "ARMCA15PLL_PWR_CON0"},
		{ARMCA7PLL_CON0, "ARMCA7PLL_CON0"},
		{ARMCA7PLL_CON1, "ARMCA7PLL_CON1"},
		{ARMCA7PLL_PWR_CON0, "ARMCA7PLL_PWR_CON0"},
		{MAINPLL_CON0, "MAINPLL_CON0"},
		{MAINPLL_CON1, "MAINPLL_CON1"},
		{MAINPLL_PWR_CON0, "MAINPLL_PWR_CON0"},
		{UNIVPLL_CON0, "UNIVPLL_CON0"},
		{UNIVPLL_CON1, "UNIVPLL_CON1"},
		{UNIVPLL_PWR_CON0, "UNIVPLL_PWR_CON0"},
		{MMPLL_CON0, "MMPLL_CON0"},
		{MMPLL_CON1, "MMPLL_CON1"},
		{MMPLL_PWR_CON0, "MMPLL_PWR_CON0"},
		{MSDCPLL_CON0, "MSDCPLL_CON0"},
		{MSDCPLL_CON1, "MSDCPLL_CON1"},
		{MSDCPLL_PWR_CON0, "MSDCPLL_PWR_CON0"},
		{VENCPLL_CON0, "VENCPLL_CON0"},
		{VENCPLL_CON1, "VENCPLL_CON1"},
		{VENCPLL_PWR_CON0, "VENCPLL_PWR_CON0"},
		{TVDPLL_CON0, "TVDPLL_CON0"},
		{TVDPLL_CON1, "TVDPLL_CON1"},
		{TVDPLL_PWR_CON0, "TVDPLL_PWR_CON0"},
		{MPLL_CON0, "MPLL_CON0"},
		{MPLL_CON1, "MPLL_CON1"},
		{MPLL_PWR_CON0, "MPLL_PWR_CON0"},
		{VCODECPLL_CON0, "VCODECPLL_CON0"},
		{VCODECPLL_CON1, "VCODECPLL_CON1"},
		{VCODECPLL_PWR_CON0, "VCODECPLL_PWR_CON0"},
		{APLL1_CON0, "APLL1_CON0"},
		{APLL1_CON1, "APLL1_CON1"},
		{APLL1_PWR_CON0, "APLL1_PWR_CON0"},
		{APLL2_CON0, "APLL2_CON0"},
		{APLL2_CON1, "APLL2_CON1"},
		{APLL2_PWR_CON0, "APLL2_PWR_CON0"},
		{INFRA_PDN0_STA, "INFRA_PDN0_STA"},
		{INFRA_PDN1_STA, "INFRA_PDN1_STA"},
		{INFRA_PDN2_STA, "INFRA_PDN2_STA"},
		{DISP_CG_CON0, "DISP_CG_CON0"},
		{DISP_CG_CON1, "DISP_CG_CON1"},
		{IMG_CG_CON, "IMG_CG_CON"},
		{VDEC_CKEN_SET, "VDEC_CKEN_SET"},
		{LARB_CKEN_SET, "LARB_CKEN_SET"},
		{VENC_CG_CON, "VENC_CG_CON"},
		{AUDIO_TOP_CON0, "AUDIO_TOP_CON0"},
		{MFG_CG_CON, "MFG_CG_CON"},
#endif /* CLKDBG_SOC */
	};

	size_t n = min(ARRAY_SIZE(rn), size);

	memcpy(regnames, rn, sizeof(rn[0]) * n);
	return n;
}

#if DUMP_INIT_STATE

static void print_reg(void __iomem *reg, const char *name)
{
	if (!is_valid_reg(reg))
		return;

	clk_info("%-21s: [0x%p] = 0x%08x\n", name, reg, clk_readl(reg));
}

static void print_regs(void)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		print_reg(rn[i].reg, rn[i].name);
}

#endif /* DUMP_INIT_STATE */

static void seq_print_reg(void __iomem *reg, const char *name,
		struct seq_file *s, void *v)
{
	u32 val;

	if (!is_valid_reg(reg))
		return;

	val = clk_readl(reg);

	clk_info("%-21s: [0x%p] = 0x%08x\n", name, reg, val);
	seq_printf(s, "%-21s: [0x%p] = 0x%08x\n", name, reg, val);
}

static int seq_print_regs(struct seq_file *s, void *v)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		seq_print_reg(rn[i].reg, rn[i].name, s, v);

	return 0;
}

#if CLKDBG_6755

/* ROOT */
#define clk_null		"clk_null"
#define clk26m			"clk26m"
#define clk32k			"clk32k"

#define clkph_mck_o		"clkph_mck_o"
#define dpi_ck			"dpi_ck"

/* PLL */
#define armbpll		"armbpll"
#define armspll		"armspll"
#define mainpll		"mainpll"
#define msdcpll		"msdcpll"
#define univpll		"univpll"
#define mmpll		"mmpll"
#define vencpll		"vencpll"
#define tvdpll		"tvdpll"
#define apll1		"apll1"
#define apll2		"apll2"
#define oscpll		"oscpll"

/* DIV */
#define syspll_ck		"syspll_ck"
#define syspll1_ck		"syspll1_ck"
#define	syspll2_ck		"syspll2_ck"
#define syspll3_ck		"syspll3_ck"
#define	syspll4_ck		"syspll4_ck"
#define univpll_ck		"univpll_ck"
#define univpll1_ck		"univpll1_ck"
#define univpll2_ck		"univpll2_ck"
#define univpll3_ck		"univpll3_ck"
#define ad_apll1_ck		"ad_apll1_ck"
#define ad_sys_26m_ck		"ad_sys_26m_ck"
#define ad_sys_26m_d2		"ad_sys_26m_d2"
#define dmpll_ck		"dmpll_ck"
#define dmpll_d2		"dmpll_d2"
#define dmpll_d4		"dmpll_d4"
#define dmpll_d8		"dmpll_d8"
#define dpi_ck		"dpi_ck"
#define mmpll_ck		"mmpll_ck"
#define msdcpll_ck		"msdcpll_ck"
#define msdcpll_d16		"msdcpll_d16"
#define msdcpll_d2		"msdcpll_d2"
#define msdcpll_d4		"msdcpll_d4"
#define msdcpll_d8		"msdcpll_d8"
#define syspll_d2		"syspll_d2"
#define syspll_d3		"syspll_d3"
#define syspll_d5		"syspll_d5"
#define syspll1_d16		"syspll1_d16"
#define syspll1_d2		"syspll1_d2"
#define syspll1_d4		"syspll1_d4"
#define syspll1_d8		"syspll1_d8"
#define syspll2_d2		"syspll2_d2"
#define syspll2_d4		"syspll2_d4"
#define syspll3_d2		"syspll3_d2"
#define syspll3_d4		"syspll3_d4"
#define syspll4_d2		"syspll4_d2"
#define syspll4_d2_d8		"syspll4_d2_d8"
#define syspll4_d4		"syspll4_d4"
#define tvdpll_ck		"tvdpll_ck"
#define tvdpll_d2		"tvdpll_d2"
#define tvdpll_d4		"tvdpll_d4"
#define univpll_d2		"univpll_d2"
#define univpll_d26		"univpll_d26"
#define univpll_d3		"univpll_d3"
#define univpll_d5		"univpll_d5"
#define univpll1_d2		"univpll1_d2"
#define univpll1_d4		"univpll1_d4"
#define univpll1_d8		"univpll1_d8"
#define univpll2_d2		"univpll2_d2"
#define univpll2_d4		"univpll2_d4"
#define univpll2_d8		"univpll2_d8"
#define univpll3_d2		"univpll3_d2"
#define univpll3_d4		"univpll3_d4"
#define vencpll_ck		"vencpll_ck"
#define vencpll_d3		"vencpll_d3"
#define whpll_audio_ck		"whpll_audio_ck"

#define osc_ck	"osc_ck"
#define osc_d8	"osc_d8"
#define osc_d2	"osc_d2"
#define syspll_d7	"syspll_d7"
#define univpll_d7	"univpll_d7"
#define osc_d4	"osc_d4"
#define	tvdpll_d8	"tvdpll_d8"
#define	tvdpll_d16	"tvdpll_d16"
#define	apll1_ck	"apll1_ck"
#define	apll2_ck	"apll2_ck"
#define	syspll_d3_d3	"syspll_d3_d3"



/* TOP */
#define axi_sel			"axi_sel"
#define mem_sel			"mem_sel"
#define ddrphycfg_sel			"ddrphycfg_sel"
#define mm_sel			"mm_sel"
#define pwm_sel			"pwm_sel"
#define vdec_sel			"vdec_sel"
#define mfg_sel			"mfg_sel"
#define camtg_sel			"camtg_sel"
#define uart_sel			"uart_sel"
#define spi_sel			"spi_sel"
#define usb20_sel			"usb20_sel"
#define msdc50_0_sel			"msdc50_0_sel"

#define msdc30_1_sel			"msdc30_1_sel"
#define msdc30_2_sel			"msdc30_2_sel"
#define msdc30_3_sel			"msdc30_3_sel"
#define audio_sel			"audio_sel"
#define aud_intbus_sel			"aud_intbus_sel"
#define pmicspi_sel			"pmicspi_sel"
#define scp_sel			"scp_sel"
#define atb_sel			"atb_sel"
#define dpi0_sel			"dpi0_sel"
#define scam_sel			"scam_sel"
#define mfg13m_sel			"mfg13m_sel"
#define aud_1_sel			"aud_1_sel"
#define aud_2_sel			"aud_2_sel"
#define irda_sel			"irda_sel"
#define irtx_sel			"irtx_sel"
#define disppwm_sel			"disppwm_sel"
#define ssusb_top_sys_sel		"ssusb_top_sys_sel"
#define usb_top_sel	"usb_top_sel"
#define i2c_sel			"i2c_sel"
#define f26m_sel		"f26m_sel"
#define msdc50_0_hclk_sel		"msdc50_0_hclk_sel"
#define	bsi_spi_sel	"bsi_spi_sel"
#define spm_sel				"spm_sel"
#define	dvfsp_sel	"dvfsp_sel"
#define i2s_sel				"i2s_sel"



/* INFRA */
#define	infra_pmictmr	"infra_pmictmr"
#define	infra_pmicap	"infra_pmicap"
#define	infra_pmicmd	"infra_pmicmd"
#define	infra_pmicconn	"infra_pmicconn"
#define	infra_scpsys	"infra_scpsys"
#define	infra_sej	"infra_sej"
#define	infra_apxgpt	"infra_apxgpt"
#define	infra_icusb	"infra_icusb"
#define	infra_gce	"infra_gce"
#define	infra_therm	"infra_therm"
#define	infra_i2c0	"infra_i2c0"
#define	infra_i2c1	"infra_i2c1"
#define	infra_i2c2	"infra_i2c2"
#define	infra_i2c3	"infra_i2c3"
#define	infra_pwmhclk	"infra_pwmhclk"
#define	infra_pwm1	"infra_pwm1"
#define	infra_pwm2	"infra_pwm2"
#define	infra_pwm3	"infra_pwm3"
#define	infra_pwm4	"infra_pwm4"
#define	infra_pwm	"infra_pwm"
#define	infra_uart0	"infra_uart0"
#define	infra_uart1	"infra_uart1"
#define	infra_uart2	"infra_uart2"
#define	infra_uart3	"infra_uart3"
#define	infra_md2mdccif0	"infra_md2mdccif0"
#define	infra_md2mdccif1	"infra_md2mdccif1"
#define	infra_md2mdccif2	"infra_md2mdccif2"
#define	infra_btif	"infra_btif"
#define	infra_md2mdccif3	"infra_md2mdccif3"
#define	infra_spi0	"infra_spi0"
#define	infra_msdc0	"infra_msdc0"
#define	infra_md2mdccif4	"infra_md2mdccif4"
#define	infra_msdc1	"infra_msdc1"
#define	infra_msdc2	"infra_msdc2"
#define	infra_msdc3	"infra_msdc3"
#define	infra_md2mdccif5	"infra_md2mdccif5"
#define	infra_gcpu	"infra_gcpu"
#define	infra_trng	"infra_trng"
#define	infra_auxadc	"infra_auxadc"
#define	infra_cpum	"infra_cpum"
#define	infra_ccif1ap	"infra_ccif1ap"
#define	infra_ccif1md	"infra_ccif1md"
#define	infra_apdma	"infra_apdma"
#define	infra_xiu	"infra_xiu"
#define	infra_deviceapc	"infra_deviceapc"
#define	infra_xiu2ahb	"infra_xiu2ahb"
#define	infra_ccifap	"infra_ccifap"
#define	infra_debugsys	"infra_debugsys"
#define	infra_audio	"infra_audio" /* confirm if for audio??*/
#define	infra_ccifmd	"infra_ccifmd"
#define	infra_dramcf26m	"infra_dramcf26m"
#define	infra_irtx	"infra_irtx"
#define	infra_ssusbtop	"infra_ssusbtop"
#define	infra_disppwm	"infra_disppwm"
#define	infra_cldmabclk	"infra_cldmabclk"
#define	infra_audio26mbclk	"infra_audio26mbclk"
#define	infra_mdtmp26mbclk	"infra_mdtmp26mbclk"
#define	infra_spi1	"infra_spi1"
#define	infra_i2c4	"infra_i2c4"
#define	infra_mdtmpshare	"infra_mdtmpshare"


/* MFG */
#define mfg_bg3d			"mfg_bg3d"

/* IMG */
#define img_image_larb2_smi		"img_image_larb2_smi"
#define img_image_larb2_smi_m4u		"img_image_larb2_smi_m4u"
#define img_image_larb2_smi_smi_common		"img_image_larb2_smi_smi_common"
#define img_image_larb2_smi_met_smi_common		"img_image_larb2_smi_met_smi_common"
#define img_image_larb2_smi_ispsys		"img_image_larb2_smi_ispsys"
#define img_image_cam_smi		"img_image_cam_smi"
#define img_image_cam_cam		"img_image_cam_cam"
#define img_image_sen_tg		"img_image_sen_tg"
#define img_image_sen_cam		"img_image_sen_cam"
#define img_image_cam_sv		"img_image_cam_sv"
#define img_image_sufod		"img_image_sufod"
#define img_image_fd		"img_image_fd"

/* MM_SYS */
#define	mm_disp0_smi_common	"mm_disp0_smi_common"
#define	mm_disp0_smi_common_m4u	"mm_disp0_smi_common_m4u"
#define	mm_disp0_smi_common_mali	"mm_disp0_smi_common_mali"
#define	mm_disp0_smi_common_dispsys	"mm_disp0_smi_common_dispsys"
#define	mm_disp0_smi_common_smi_common	"mm_disp0_smi_common_smi_common"
#define	mm_disp0_smi_common_met_smi_common	"mm_disp0_smi_common_met_smi_common"
#define	mm_disp0_smi_common_ispsys	"mm_disp0_smi_common_ispsys"
#define	mm_disp0_smi_common_fdvt	"mm_disp0_smi_common_fdvt"
#define	mm_disp0_smi_common_vdec_gcon	"mm_disp0_smi_common_vdec_gcon"
#define	mm_disp0_smi_common_jpgenc	"mm_disp0_smi_common_jpgenc"
#define	mm_disp0_smi_common_jpgdec	"mm_disp0_smi_common_jpgdec"

#define	mm_disp0_smi_larb0	"mm_disp0_smi_larb0"
#define	mm_disp0_smi_larb0_m4u	"mm_disp0_smi_larb0_m4u"
#define	mm_disp0_smi_larb0_dispsys	"mm_disp0_smi_larb0_dispsys"
#define	mm_disp0_smi_larb0_smi_common	"mm_disp0_smi_larb0_smi_common"
#define	mm_disp0_smi_larb0_met_smi_common	"mm_disp0_smi_larb0_met_smi_common"


#define	mm_disp0_cam_mdp	"mm_disp0_cam_mdp"
#define	mm_disp0_mdp_rdma	"mm_disp0_mdp_rdma"
#define	mm_disp0_mdp_rsz0	"mm_disp0_mdp_rsz0"
#define	mm_disp0_mdp_rsz1	"mm_disp0_mdp_rsz1"
#define	mm_disp0_mdp_tdshp	"mm_disp0_mdp_tdshp"
#define	mm_disp0_mdp_wdma	"mm_disp0_mdp_wdma"
#define	mm_disp0_mdp_wrot	"mm_disp0_mdp_wrot"
#define	mm_disp0_fake_eng	"mm_disp0_fake_eng"
#define	mm_disp0_disp_ovl0	"mm_disp0_disp_ovl0"
#define	mm_disp0_disp_ovl1	"mm_disp0_disp_ovl1"
#define	mm_disp0_disp_rdma0	"mm_disp0_disp_rdma0"
#define	mm_disp0_disp_rdma1	"mm_disp0_disp_rdma1"
#define	mm_disp0_disp_wdma0	"mm_disp0_disp_wdma0"
#define	mm_disp0_disp_color	"mm_disp0_disp_color"
#define	mm_disp0_disp_ccorr	"mm_disp0_disp_ccorr"
#define	mm_disp0_disp_aal	"mm_disp0_disp_aal"
#define	mm_disp0_disp_gamma	"mm_disp0_disp_gamma"
#define	mm_disp0_disp_dither	"mm_disp0_disp_dither"
#define	mm_disp0_mdp_color	"mm_disp0_mdp_color"
#define	mm_disp0_ufoe_mout	"mm_disp0_ufoe_mout"
#define	mm_disp0_disp_wdma1	"mm_disp0_disp_wdma1"
#define	mm_disp0_disp_2lovl0	"mm_disp0_disp_2lovl0"
#define	mm_disp0_disp_2lovl1	"mm_disp0_disp_2lovl1"
#define	mm_disp0_disp_ovl0mout	"mm_disp0_disp_ovl0mout"
#define	mm_disp1_dsi_engine	"mm_disp1_dsi_engine"
#define	mm_disp1_dsi_digital	"mm_disp1_dsi_digital"
#define	mm_disp1_dpi_pixel	"mm_disp1_dpi_pixel"
#define	mm_disp1_dpi_engine	"mm_disp1_dpi_engine"

/* VDEC */
#define vdec0_vdec		"vdec0_vdec"
#define vdec1_larb		"vdec1_larb"
#define vdec1_larb_m4u		"vdec1_larb_m4u"
#define vdec1_larb_smi_common		"vdec1_larb_smi_common"
#define vdec1_larb_met_smi_common		"vdec1_larb_met_smi_common"
#define vdec1_larb_vdec_gcon		"vdec1_larb_vdec_gcon"

/* VENC */
#define venc_larb		"venc_larb"
#define venc_larb_m4u		"venc_larb_m4u"
#define venc_larb_smi_common		"venc_larb_smi_common"
#define venc_larb_met_smi_common		"venc_larb_met_smi_common"
#define venc_larb_jpgenc		"venc_larb_jpgenc"
#define venc_larb_jpgdec		"venc_larb_jpgdec"

#define venc_venc		"venc_venc"
#define venc_jpgenc		"venc_jpgenc"
#define venc_jpgdec		"venc_jpgdec"

/* AUDIO */
#define audio_afe		"audio_afe"
#define audio_i2s		"audio_i2s"
#define audio_22m		"audio_22m"
#define audio_24m		"audio_24m"
#define audio_apll2_tuner		"audio_apll2_tuner"
#define audio_apll_tuner		"audio_apll_tuner"
#define audio_adc		"audio_adc"
#define audio_dac		"audio_dac"
#define audio_dac_predis		"audio_dac_predis"
#define audio_tml		"audio_tml"
#define	audio_apll1div0	"audio_apll1div0"
#define	audio_apll2div0	"audio_apll2div0"

/* SCP */
#define pg_md1			"pg_md1"
#define pg_md2			"pg_md2"
#define pg_conn			"pg_conn"
#define pg_dis			"pg_dis"
#define pg_mfg			"pg_mfg"
#define pg_isp			"pg_isp"
#define pg_vde			"pg_vde"
#define pg_ven			"pg_ven"
#define pg_aud		"pg_aud"
/*#define pg_mfg_async		"pg_mfg_async"*/

#endif /* CLKDBG_6755 */

static const char * const *get_all_clk_names(size_t *num)
{
	static const char * const clks[] = {
#if CLKDBG_6755
		/* ROOT */
		clk_null,
		clk26m,
		clk32k,

		clkph_mck_o,

		/* PLL */
		armbpll,
		armspll,
		mainpll,
		msdcpll,
		univpll,
		mmpll,
		vencpll,
		tvdpll,
		apll1,
		apll2,
		oscpll,

		/* DIV */
		syspll_ck,
		syspll2_ck,
		syspll3_ck,
		syspll4_ck,
		univpll_ck,
		univpll1_ck,
		univpll2_ck,
		univpll3_ck,
		ad_apll1_ck,
		ad_sys_26m_ck,
		ad_sys_26m_d2,
		dmpll_ck,
		dmpll_d2,
		dmpll_d4,
		dmpll_d8,
		dpi_ck,
		mmpll_ck,
		msdcpll_ck,
		msdcpll_d16,
		msdcpll_d2,
		msdcpll_d4,
		msdcpll_d8,
		syspll_d2,
		syspll_d3,
		syspll_d5,
		syspll1_d16,
		syspll1_d2,
		syspll1_d4,
		syspll1_d8,
		syspll2_d2,
		syspll2_d4,
		syspll3_d2,
		syspll3_d4,
		syspll4_d2,
		syspll4_d2_d8,
		syspll4_d4,
		tvdpll_ck,
		tvdpll_d2,
		tvdpll_d4,
		univpll_d2,
		univpll_d26,
		univpll_d3,
		univpll_d5,
		univpll1_d2,
		univpll1_d4,
		univpll1_d8,
		univpll2_d2,
		univpll2_d4,
		univpll2_d8,
		univpll3_d2,
		univpll3_d4,
		vencpll_ck,
		vencpll_d3,
		whpll_audio_ck,
		osc_ck,
		osc_d8,
		osc_d2,
		syspll_d7,
		univpll_d7,
		osc_d4,
		tvdpll_d8,
		tvdpll_d16,
		apll1_ck,
		apll2_ck,
		syspll_d3_d3,

		/* TOP */
		axi_sel,
		mem_sel,
		ddrphycfg_sel,
		mm_sel,
		pwm_sel,
		vdec_sel,
		mfg_sel,
		camtg_sel,
		uart_sel,
		spi_sel,
		usb20_sel,
		msdc50_0_sel,
		msdc30_1_sel,
		msdc30_2_sel,
		msdc30_3_sel,
		audio_sel,
		aud_intbus_sel,
		pmicspi_sel,
		scp_sel,
		atb_sel,
		dpi0_sel,
		scam_sel,
		mfg13m_sel,
		aud_1_sel,
		aud_2_sel,
		irda_sel,
		irtx_sel,
		disppwm_sel,
		ssusb_top_sys_sel,
		usb_top_sel,
		i2c_sel,
		f26m_sel,
		msdc50_0_hclk_sel,
		bsi_spi_sel,
		spm_sel,
		dvfsp_sel,
		i2s_sel,


		/* INFRA */
		infra_pmictmr,
		infra_pmicap,
		infra_pmicmd,
		infra_pmicconn,
		infra_scpsys,
		infra_sej,
		infra_apxgpt,
		infra_icusb,
		infra_gce,
		infra_therm,
		infra_i2c0,
		infra_i2c1,
		infra_i2c2,
		infra_i2c3,
		infra_pwmhclk,
		infra_pwm1,
		infra_pwm2,
		infra_pwm3,
		infra_pwm4,
		infra_pwm,
		infra_uart0,
		infra_uart1,
		infra_uart2,
		infra_uart3,
		infra_md2mdccif0,
		infra_md2mdccif1,
		infra_md2mdccif2,
		infra_btif,
		infra_md2mdccif3,
		infra_spi0,
		infra_msdc0,
		infra_md2mdccif4,
		infra_msdc1,
		infra_msdc2,
		infra_msdc3,
		infra_md2mdccif5,
		infra_gcpu,

		/* MFG */
		mfg_bg3d,

		/* IMG */
		img_image_larb2_smi,
		img_image_larb2_smi_m4u,
		img_image_larb2_smi_smi_common,
		img_image_larb2_smi_met_smi_common,
		img_image_larb2_smi_ispsys,
		img_image_cam_smi,
		img_image_cam_cam,
		img_image_sen_tg,
		img_image_sen_cam,
		img_image_cam_sv,
		img_image_sufod,
		img_image_fd,


		/* MM */
		mm_disp0_smi_common,
		mm_disp0_smi_common_m4u,
		mm_disp0_smi_common_mali,
		mm_disp0_smi_common_dispsys,
		mm_disp0_smi_common_smi_common,
		mm_disp0_smi_common_met_smi_common,
		mm_disp0_smi_common_ispsys,
		mm_disp0_smi_common_fdvt,
		mm_disp0_smi_common_vdec_gcon,
		mm_disp0_smi_common_jpgenc,
		mm_disp0_smi_common_jpgdec,
		mm_disp0_smi_larb0,
		mm_disp0_smi_larb0_m4u,
		mm_disp0_smi_larb0_dispsys,
		mm_disp0_smi_larb0_smi_common,
		mm_disp0_smi_larb0_met_smi_common,
		mm_disp0_cam_mdp,
		mm_disp0_mdp_rdma,
		mm_disp0_mdp_rsz0,
		mm_disp0_mdp_rsz1,
		mm_disp0_mdp_tdshp,
		mm_disp0_mdp_wdma,
		mm_disp0_mdp_wrot,
		mm_disp0_fake_eng,
		mm_disp0_disp_ovl0,
		mm_disp0_disp_ovl1,
		mm_disp0_disp_rdma0,
		mm_disp0_disp_rdma1,
		mm_disp0_disp_wdma0,
		mm_disp0_disp_color,
		mm_disp0_disp_ccorr,
		mm_disp0_disp_aal,
		mm_disp0_disp_gamma,
		mm_disp0_disp_dither,
		mm_disp0_mdp_color,
		mm_disp0_ufoe_mout,
		mm_disp0_disp_wdma1,
		mm_disp0_disp_2lovl0,
		mm_disp0_disp_2lovl1,
		mm_disp0_disp_ovl0mout,
		mm_disp1_dsi_engine,
		mm_disp1_dsi_digital,
		mm_disp1_dpi_pixel,
		mm_disp1_dpi_engine,




		/* VDEC */
		vdec0_vdec,
		vdec1_larb,
		vdec1_larb_m4u,
		vdec1_larb_smi_common,
		vdec1_larb_met_smi_common,
		vdec1_larb_vdec_gcon,


		/* VENC */
		venc_larb,
		venc_larb_m4u,
		venc_larb_smi_common,
		venc_larb_met_smi_common,
		venc_larb_jpgenc,
		venc_larb_jpgdec,
		venc_venc,
		venc_jpgenc,
		venc_jpgdec,

		/* AUDIO */
		audio_afe,
		audio_i2s,
		audio_22m,
		audio_24m,
		audio_apll2_tuner,
		audio_apll_tuner,
		audio_adc,
		audio_dac,
		audio_dac_predis,
		audio_tml,
		audio_apll1div0,
		audio_apll2div0,


		pg_md1,
		pg_md2,
		pg_conn,
		pg_dis,
		pg_mfg,
		pg_isp,
		pg_vde,
		pg_ven,
		pg_aud,
		/*pg_mfg_async,*/


#endif /* CLKDBG_SOC */
	};

	*num = ARRAY_SIZE(clks);

	return clks;
}

static void dump_clk_state(const char *clkname, struct seq_file *s)
{
	struct clk *c = __clk_lookup(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : __clk_get_parent(c);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		__clk_get_name(c),
		(__clk_is_enabled(c) || __clk_is_prepared(c)) ? "ON" : "off",
		__clk_is_prepared(c),
		__clk_get_enable_count(c),
		__clk_get_rate(c),
		p ? __clk_get_name(p) : "");
}

static int clkdbg_dump_state_all(struct seq_file *s, void *v)
{
	int i;
	size_t num;

	const char * const *clks = get_all_clk_names(&num);

	pr_debug("\n");
	for (i = 0; i < num; i++)
		dump_clk_state(clks[i], s);

	return 0;
}

static char last_cmd[128] = "null";

static int clkdbg_prepare_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_prepare_enable(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_prepare_enable(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_disable_unprepare(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_disable_unprepare(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	clk_disable_unprepare(clk);
	seq_puts(s, "0\n");

	return 0;
}

static int clkdbg_set_parent(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *parent_name;
	struct clk *clk;
	struct clk *parent;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	parent_name = strsep(&c, " ");

	seq_printf(s, "clk_set_parent(%s, %s): ", clk_name, parent_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	parent = __clk_lookup(parent_name);
	if (IS_ERR_OR_NULL(parent)) {
		seq_printf(s, "clk_get(): 0x%p\n", parent);
		return PTR_ERR(parent);
	}

	clk_prepare_enable(clk);
	r = clk_set_parent(clk, parent);
	clk_disable_unprepare(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_set_rate(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *rate_str;
	struct clk *clk;
	unsigned long rate;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	rate_str = strsep(&c, " ");
	r = kstrtoul(rate_str, 0, &rate);

	seq_printf(s, "clk_set_rate(%s, %lu): %d: ", clk_name, rate, r);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_set_rate(clk, rate);
	seq_printf(s, "%d\n", r);

	return r;
}

void *reg_from_str(const char *str)
{
	if (sizeof(void *) == sizeof(unsigned long)) {
		unsigned long v;

		if (kstrtoul(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else if (sizeof(void *) == sizeof(unsigned long long)) {
		unsigned long long v;

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else {
		unsigned long long v;

		clk_warn("unexpected pointer size: sizeof(void *): %zu\n",
			sizeof(void *));

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	}

	clk_warn("%s(): parsing error: %s\n", __func__, str);

	return NULL;
}

static int parse_reg_val_from_cmd(void __iomem **preg, unsigned long *pval)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *reg_str;
	char *val_str;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	reg_str = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (preg)
		*preg = reg_from_str(reg_str);

	if (pval)
		r = kstrtoul(val_str, 0, pval);

	return r;
}

static int clkdbg_reg_read(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, NULL);
	seq_printf(s, "readl(0x%p): ", reg);

	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_write(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_writel(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_set(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_setl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_clr(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_clrl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

#if CLKDBG_PM_DOMAIN

/*
 * pm_domain support
 */

static struct generic_pm_domain **get_all_pm_domain(int *numpd)
{
	static struct generic_pm_domain *pds[10];
	const int maxpd = ARRAY_SIZE(pds);
#if CLKDBG_6755
	const char *cmp = "mediatek,mt6755-scpsys";
#endif

	struct device_node *node;
	int i;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_err("node '%s' not found!\n", cmp);
		return NULL;
	}

	for (i = 0; i < maxpd; i++) {
		struct of_phandle_args pa;

		pa.np = node;
		pa.args[0] = i;
		pa.args_count = 1;
		pds[i] = of_genpd_get_from_provider(&pa);

		if (IS_ERR(pds[i]))
			break;
	}

	if (numpd)
		*numpd = i;

	return pds;
}

static struct generic_pm_domain *genpd_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		if (strcmp(name, pd->name) == 0)
			return pd;
	}

	return NULL;
}

static struct platform_device *pdev_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			if (strcmp(name, pdev->name) == 0)
				return pdev;
		}
	}

	return NULL;
}

static void seq_print_all_genpd(struct seq_file *s)
{
	static const char * const gpd_status_name[] = {
		"ACTIVE",
		"WAIT_MASTER",
		"BUSY",
		"REPEAT",
		"POWER_OFF",
	};

	static const char * const prm_status_name[] = {
		"active",
		"resuming",
		"suspended",
		"suspending",
	};

	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	seq_puts(s, "domain_on [pmd_name  status]\n");
	seq_puts(s, "\tdev_on (dev_name usage_count, disable, status)\n");
	seq_puts(s, "------------------------------------------------------\n");

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd)) {
			seq_printf(s, "pd[%d]: 0x%p\n", i, pd);
			continue;
		}

		seq_printf(s, "%c [%-9s %11s]\n",
			(pd->status == GPD_STATE_ACTIVE) ? '+' : '-',
			pd->name, gpd_status_name[pd->status]);

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			seq_printf(s, "\t%c (%-16s %3d, %d, %10s)\n",
				pm_runtime_active(dev) ? '+' : '-',
				pdev->name,
				atomic_read(&dev->power.usage_count),
				dev->power.disable_depth,
				prm_status_name[dev->power.runtime_status]);
		}
	}
}

static int clkdbg_dump_genpd(struct seq_file *s, void *v)
{
	seq_print_all_genpd(s);

	return 0;
}

static int clkdbg_pm_genpd_poweron(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *genpd_name;
	struct generic_pm_domain *pd;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	genpd_name = strsep(&c, " ");

	seq_printf(s, "pm_genpd_poweron(%s): ", genpd_name);

	pd = genpd_from_name(genpd_name);
	if (pd) {
		int r = pm_genpd_poweron(pd);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_genpd_poweroff_unused(struct seq_file *s, void *v)
{
	seq_puts(s, "pm_genpd_poweroff_unused()\n");
	pm_genpd_poweroff_unused();

	return 0;
}

static int clkdbg_pm_runtime_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_enable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_enable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_disable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_disable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_disable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_get_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_get_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_get_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_put_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_put_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_put_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

#endif /* CLKDBG_PM_DOMAIN */

#define CMDFN(_cmd, _fn) {	\
	.cmd = _cmd,		\
	.fn = _fn,		\
}

static int clkdbg_show(struct seq_file *s, void *v)
{
	static const struct {
		const char	*cmd;
		int (*fn)(struct seq_file *, void *);
	} cmds[] = {
		CMDFN("dump_regs", seq_print_regs),
		CMDFN("dump_state", clkdbg_dump_state_all),
		CMDFN("fmeter", seq_print_fmeter_all),
		CMDFN("prepare_enable", clkdbg_prepare_enable),
		CMDFN("disable_unprepare", clkdbg_disable_unprepare),
		CMDFN("set_parent", clkdbg_set_parent),
		CMDFN("set_rate", clkdbg_set_rate),
		CMDFN("reg_read", clkdbg_reg_read),
		CMDFN("reg_write", clkdbg_reg_write),
		CMDFN("reg_set", clkdbg_reg_set),
		CMDFN("reg_clr", clkdbg_reg_clr),
#if CLKDBG_PM_DOMAIN
		CMDFN("dump_genpd", clkdbg_dump_genpd),
		CMDFN("pm_genpd_poweron", clkdbg_pm_genpd_poweron),
		CMDFN("pm_genpd_poweroff_unused",
			clkdbg_pm_genpd_poweroff_unused),
		CMDFN("pm_runtime_enable", clkdbg_pm_runtime_enable),
		CMDFN("pm_runtime_disable", clkdbg_pm_runtime_disable),
		CMDFN("pm_runtime_get_sync", clkdbg_pm_runtime_get_sync),
		CMDFN("pm_runtime_put_sync", clkdbg_pm_runtime_put_sync),
#endif /* CLKDBG_PM_DOMAIN */
	};

	int i;
	char cmd[sizeof(last_cmd) + 1];

	pr_debug("last_cmd: %s\n", last_cmd);

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cmds[i].cmd, token) == 0)
			return cmds[i].fn(s, v);
	}

	return 0;
}

static int clkdbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, clkdbg_show, NULL);
}

static ssize_t clkdbg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
{
	char desc[sizeof(last_cmd) - 1];
	int len = 0;

	pr_debug("count: %zu\n", count);
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	strncpy(last_cmd, desc, sizeof(last_cmd));
	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations clkdbg_fops = {
	.owner		= THIS_MODULE,
	.open		= clkdbg_open,
	.read		= seq_read,
	.write		= clkdbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

static int __init get_base_from_node(
			const char *cmp, int idx, void __iomem **pbase)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_warn("node '%s' not found!\n", cmp);
		return -1;
	}

	*pbase = get_reg(node, idx);
	clk_info("%s base: 0x%p\n", cmp, *pbase);

	return 0;
}

static void __init init_iomap(void)
{
#if CLKDBG_6755
	get_base_from_node("mediatek,mt6755-topckgen", 0, &topckgen_base);
	get_base_from_node("mediatek,mt6755-infrasys", 0, &infrasys_base);
	get_base_from_node("mediatek,mt6755-perisys", 0, &perisys_base);
	get_base_from_node("mediatek,mcucfg", 0, &mcucfg_base);
	get_base_from_node("mediatek,mt6755-apmixedsys", 0, &apmixedsys_base);
	get_base_from_node("mediatek,mt6755-mfgsys", 0, &mfgsys_base);
	get_base_from_node("mediatek,mt6755-mmsys", 0, &mmsys_base);
	get_base_from_node("mediatek,mt6755-imgsys", 0, &imgsys_base);
	get_base_from_node("mediatek,mt6755-vdecsys", 0, &vdecsys_base);
	get_base_from_node("mediatek,mt6755-vencsys", 0, &vencsys_base);
	get_base_from_node("mediatek,mt6755-audiosys", 0, &audiosys_base);
	get_base_from_node("mediatek,mt6755-scpsys", 1, &scpsys_base);
#endif /* CLKDBG_SOC */
}

/*
 * clkdbg pm_domain support
 */

static const struct of_device_id clkdbg_id_table[] = {
	{ .compatible = "mediatek,clkdbg-pd",},
	{ },
};
MODULE_DEVICE_TABLE(of, clkdbg_id_table);

static int clkdbg_probe(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_remove(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static const struct dev_pm_ops clkdbg_pd_pm_ops = {
	.runtime_suspend = clkdbg_pd_runtime_suspend,
	.runtime_resume = clkdbg_pd_runtime_resume,
};

static struct platform_driver clkdbg_driver = {
	.probe		= clkdbg_probe,
	.remove		= clkdbg_remove,
	.driver		= {
		.name	= "clkdbg",
		.owner	= THIS_MODULE,
		.of_match_table = clkdbg_id_table,
		.pm = &clkdbg_pd_pm_ops,
	},
};

module_platform_driver(clkdbg_driver);

/*
 * pm_domain sample code
 */

static struct platform_device *my_pdev;

int power_on_before_work(void)
{
	return pm_runtime_get_sync(&my_pdev->dev);
}

int power_off_after_work(void)
{
	return pm_runtime_put_sync(&my_pdev->dev);
}

static const struct of_device_id my_id_table[] = {
	{ .compatible = "mediatek,my-device",},
	{ },
};
MODULE_DEVICE_TABLE(of, my_id_table);

static int my_probe(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);

	my_pdev = pdev;

	return 0;
}

static int my_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver my_driver = {
	.probe		= my_probe,
	.remove		= my_remove,
	.driver		= {
		.name	= "my_driver",
		.owner	= THIS_MODULE,
		.of_match_table = my_id_table,
	},
};

module_platform_driver(my_driver);

/*
 * init functions
 */

int __init mt_clkdbg_init(void)
{
	init_iomap();

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
arch_initcall(mt_clkdbg_init);

int __init mt_clkdbg_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("clkdbg", 0, 0, &clkdbg_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
module_init(mt_clkdbg_debug_init);
