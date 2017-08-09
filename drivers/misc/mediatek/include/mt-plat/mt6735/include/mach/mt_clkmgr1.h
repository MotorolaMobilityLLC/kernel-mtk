#ifndef _MT_CLKMGR1_H
#define _MT_CLKMGR1_H

#include <linux/list.h>
/* #include "mach/mt_reg_base.h" */
/* #include "mach/mt_typedefs.h" */

#define CONFIG_CLKMGR_STAT
#define PLL_CLK_LINK
#define CLKMGR_INCFILE_VER "CLKMGR_INCFILE_D1_CCF"

/*
#define APMIXED_BASE      (0x10209000)
#define CKSYS_BASE        (0x10210000)
#define INFRACFG_AO_BASE  (0x10000000)
#define PERICFG_BASE      (0x10002000)
#define AUDIO_BASE        (0x11220000)
#define MFGCFG_BASE       (0x13000000)
#define MMSYS_CONFIG_BASE (0x14000000)
#define IMGSYS_BASE       (0x15000000)
#define VDEC_GCON_BASE    (0x16000000)
#define MJC_CONFIG_BASE   (0xF2000000)
#define VENC_GCON_BASE    (0x17000000)
*/
#ifdef CONFIG_OF
extern void __iomem *clk_apmixed_base;
extern void __iomem *clk_cksys_base;
extern void __iomem *clk_infracfg_ao_base;
extern void __iomem *clk_pericfg_base;
extern void __iomem *clk_audio_base;
extern void __iomem *clk_mfgcfg_base;
extern void __iomem *clk_mmsys_config_base;
extern void __iomem *clk_imgsys_base;
extern void __iomem *clk_vdec_gcon_base;
/* extern void __iomem  *clk_mjc_config_base; */
extern void __iomem *clk_venc_gcon_base;
#endif


/* APMIXEDSYS Register */
#define AP_PLL_CON0             (clk_apmixed_base + 0x00)
#define AP_PLL_CON1             (clk_apmixed_base + 0x04)
#define AP_PLL_CON2             (clk_apmixed_base + 0x08)
#define AP_PLL_CON3             (clk_apmixed_base + 0x0C)
#define AP_PLL_CON4             (clk_apmixed_base + 0x10)
#define AP_PLL_CON5             (clk_apmixed_base + 0x14)
#define AP_PLL_CON6             (clk_apmixed_base + 0x18)
#define AP_PLL_CON7             (clk_apmixed_base + 0x1C)
#define CLKSQ_STB_CON0          (clk_apmixed_base + 0x20)
#define PLL_PWR_CON0            (clk_apmixed_base + 0x24)
#define PLL_PWR_CON1            (clk_apmixed_base + 0x28)
#define PLL_ISO_CON0            (clk_apmixed_base + 0x2C)
#define PLL_ISO_CON1            (clk_apmixed_base + 0x30)
#define PLL_STB_CON0            (clk_apmixed_base + 0x34)
#define DIV_STB_CON0            (clk_apmixed_base + 0x38)
#define PLL_CHG_CON0            (clk_apmixed_base + 0x3C)
#define PLL_TEST_CON0           (clk_apmixed_base + 0x40)

#define ARMPLL_CON0             (clk_apmixed_base + 0x200)
#define ARMPLL_CON1             (clk_apmixed_base + 0x204)
#define ARMPLL_CON2             (clk_apmixed_base + 0x208)
#define ARMPLL_PWR_CON0         (clk_apmixed_base + 0x20C)

#define MAINPLL_CON0            (clk_apmixed_base + 0x210)
#define MAINPLL_CON1            (clk_apmixed_base + 0x214)
#define MAINPLL_PWR_CON0        (clk_apmixed_base + 0x21C)

#define UNIVPLL_CON0            (clk_apmixed_base + 0x220)
#define UNIVPLL_CON1            (clk_apmixed_base + 0x224)
#define UNIVPLL_PWR_CON0        (clk_apmixed_base + 0x22C)

#define MMPLL_CON0              (clk_apmixed_base + 0x230)
#define MMPLL_CON1              (clk_apmixed_base + 0x234)
#define MMPLL_CON2              (clk_apmixed_base + 0x238)
#define MMPLL_PWR_CON0          (clk_apmixed_base + 0x23C)

#define MSDCPLL_CON0            (clk_apmixed_base + 0x240)
#define MSDCPLL_CON1            (clk_apmixed_base + 0x244)
#define MSDCPLL_PWR_CON0        (clk_apmixed_base + 0x24C)

#define VENCPLL_CON0            (clk_apmixed_base + 0x250)
#define VENCPLL_CON1            (clk_apmixed_base + 0x254)
#define VENCPLL_PWR_CON0        (clk_apmixed_base + 0x25C)

#define TVDPLL_CON0             (clk_apmixed_base + 0x260)
#define TVDPLL_CON1             (clk_apmixed_base + 0x264)
#define TVDPLL_PWR_CON0         (clk_apmixed_base + 0x26C)

/* #define MPLL_CON0               (clk_apmixed_base + 0x280) */
/* #define MPLL_CON1               (clk_apmixed_base + 0x284) */
/* #define MPLL_PWR_CON0           (clk_apmixed_base + 0x28C) */

#define APLL1_CON0              (clk_apmixed_base + 0x270)
#define APLL1_CON1              (clk_apmixed_base + 0x274)
#define APLL1_CON2              (clk_apmixed_base + 0x278)
#define APLL1_CON3              (clk_apmixed_base + 0x27C)
#define APLL1_PWR_CON0          (clk_apmixed_base + 0x280)

#define APLL2_CON0              (clk_apmixed_base + 0x284)
#define APLL2_CON1              (clk_apmixed_base + 0x288)
#define APLL2_CON2              (clk_apmixed_base + 0x28C)
#define APLL2_CON3              (clk_apmixed_base + 0x290)
#define APLL2_PWR_CON0          (clk_apmixed_base + 0x294)

/* TOPCKGEN Register */
#define CLK_MODE                (clk_cksys_base + 0x000)
/* #define CLK_CFG_UPDATE          (clk_cksys_base + 0x004) */
#define TST_SEL_0               (clk_cksys_base + 0x020)
#define TST_SEL_1               (clk_cksys_base + 0x024)
/* #define TST_SEL_2               (clk_cksys_base + 0x028) */
#define CLK_CFG_0               (clk_cksys_base + 0x040)
#define CLK_CFG_1               (clk_cksys_base + 0x050)
#define CLK_CFG_2               (clk_cksys_base + 0x060)
#define CLK_CFG_3               (clk_cksys_base + 0x070)
#define CLK_CFG_4               (clk_cksys_base + 0x080)
#define CLK_CFG_5               (clk_cksys_base + 0x090)
#define CLK_CFG_6               (clk_cksys_base + 0x0A0)
#define CLK_CFG_7               (clk_cksys_base + 0x0B0)
#define CLK_CFG_8               (clk_cksys_base + 0x100)
#define CLK_CFG_9               (clk_cksys_base + 0x104)
#define CLK_CFG_10              (clk_cksys_base + 0x108)
#define CLK_CFG_11              (clk_cksys_base + 0x10C)
#define CLK_SCP_CFG_0           (clk_cksys_base + 0x200)
#define CLK_SCP_CFG_1           (clk_cksys_base + 0x204)
#define CLK_MISC_CFG_0          (clk_cksys_base + 0x210)
#define CLK_MISC_CFG_1          (clk_cksys_base + 0x214)
#define CLK_MISC_CFG_2          (clk_cksys_base + 0x218)
#define CLK26CALI_0             (clk_cksys_base + 0x220)
#define CLK26CALI_1             (clk_cksys_base + 0x224)
#define CLK26CALI_2             (clk_cksys_base + 0x228)
#define CKSTA_REG               (clk_cksys_base + 0x22C)
#define MBIST_CFG_0             (clk_cksys_base + 0x308)
#define MBIST_CFG_1             (clk_cksys_base + 0x30C)

/* INFRASYS Register */
#define TOP_CKMUXSEL            (clk_infracfg_ao_base + 0x00)
#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)

#define INFRA_PDN_SET0          (clk_infracfg_ao_base + 0x0040)
#define INFRA_PDN_CLR0          (clk_infracfg_ao_base + 0x0044)
#define INFRA_PDN_STA0          (clk_infracfg_ao_base + 0x0048)

#define TOPAXI_PROT_EN          (clk_infracfg_ao_base + 0x0220)
#define TOPAXI_PROT_STA1        (clk_infracfg_ao_base + 0x0228)
#define C2K_SPM_CTRL            (clk_infracfg_ao_base + 0x0338)

#define PERI_PDN_SET0           (clk_pericfg_base + 0x0008)
#define PERI_PDN_CLR0           (clk_pericfg_base + 0x0010)
#define PERI_PDN_STA0           (clk_pericfg_base + 0x0018)
#define PERI_GLOBALCON_CKSEL    (clk_pericfg_base + 0x005c)

/* Audio Register*/
#define AUDIO_TOP_CON0          (clk_audio_base + 0x0000)
#define AUDIO_TOP_CON1          (clk_audio_base + 0x0004)

/* MFGCFG Register*/
#define MFG_CG_CON              (clk_mfgcfg_base + 0)
#define MFG_CG_SET              (clk_mfgcfg_base + 4)
#define MFG_CG_CLR              (clk_mfgcfg_base + 8)

/* MMSYS Register*/
#define DISP_CG_CON0            (clk_mmsys_config_base + 0x100)
#define DISP_CG_SET0            (clk_mmsys_config_base + 0x104)
#define DISP_CG_CLR0            (clk_mmsys_config_base + 0x108)
#define DISP_CG_CON1            (clk_mmsys_config_base + 0x110)
#define DISP_CG_SET1            (clk_mmsys_config_base + 0x114)
#define DISP_CG_CLR1            (clk_mmsys_config_base + 0x118)

#define MMSYS_DUMMY             (clk_mmsys_config_base + 0x894)
/* #define       SMI_LARB_BWL_EN_REG     (clk_mmsys_config_base + 0x21050) */

/* IMGSYS Register */
#define IMG_CG_CON              (clk_imgsys_base + 0x0000)
#define IMG_CG_SET              (clk_imgsys_base + 0x0004)
#define IMG_CG_CLR              (clk_imgsys_base + 0x0008)

/* VDEC Register */
#define VDEC_CKEN_SET           (clk_vdec_gcon_base + 0x0000)
#define VDEC_CKEN_CLR           (clk_vdec_gcon_base + 0x0004)
#define LARB_CKEN_SET           (clk_vdec_gcon_base + 0x0008)
#define LARB_CKEN_CLR           (clk_vdec_gcon_base + 0x000C)

/* MJC Register*/
/* #define MJC_CG_CON              (clk_mjc_config_base + 0x0000) */
/* #define MJC_CG_SET              (clk_mjc_config_base + 0x0004) */
/* #define MJC_CG_CLR              (clk_mjc_config_base + 0x0008) */

/* VENC Register*/
#define VENC_CG_CON             (clk_venc_gcon_base + 0x0)
#define VENC_CG_SET             (clk_venc_gcon_base + 0x4)
#define VENC_CG_CLR             (clk_venc_gcon_base + 0x8)

/* MCUSYS Register */
/* #define IR_ROSC_CTL             (MCUCFG_BASE + 0x030) */

enum {
	MT_LARB_DISP = 0,
	MT_LARB_VDEC = 1,
	MT_LARB_IMG = 2,
	MT_LARB_VENC = 3,
	/* MT_LARB_MJC  = 4, */
};

/* larb monitor mechanism definition*/
enum {
	LARB_MONITOR_LEVEL_HIGH = 10,
	LARB_MONITOR_LEVEL_MEDIUM = 20,
	LARB_MONITOR_LEVEL_LOW = 30,
};

struct larb_monitor {
	struct list_head link;
	int level;
	void (*backup)(struct larb_monitor *h, int larb_idx);	/* called before disable larb clock */
	void (*restore)(struct larb_monitor *h, int larb_idx);	/* called after enable larb clock */
};

enum monitor_clk_sel_0 {
	no_clk_0 = 0,
	AD_UNIV_624M_CK = 5,
	AD_UNIV_416M_CK = 6,
	AD_UNIV_249P6M_CK = 7,
	AD_UNIV_178P3M_CK_0 = 8,
	AD_UNIV_48M_CK = 9,
	AD_USB_48M_CK = 10,
	rtc32k_ck_i_0 = 20,
	AD_SYS_26M_CK_0 = 21,
};
enum monitor_clk_sel {
	no_clk = 0,
	AD_SYS_26M_CK = 1,
	rtc32k_ck_i = 2,
	clkph_MCLK_o = 7,
	AD_DPICLK = 8,
	AD_MSDCPLL_CK = 9,
	AD_MMPLL_CK = 10,
	AD_UNIV_178P3M_CK = 11,
	AD_MAIN_H156M_CK = 12,
	AD_VENCPLL_CK = 13,
};

enum ckmon_sel {
	clk_ckmon0 = 0,
	clk_ckmon1 = 1,
	clk_ckmon2 = 2,
	clk_ckmon3 = 3,
};

/* enum idle_mode {
	dpidle = 0,
	soidle = 1,
	slidle = 2,
}; */

extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);
extern void print_grp_regs(void);

/* clock API */
/* extern int enable_clock(enum cg_clk_id id, char *mod_name); */
/* extern int disable_clock(enum cg_clk_id id, char *mod_name); */
/* extern int mt_enable_clock(enum cg_clk_id id, char *mod_name); */
/* extern int mt_disable_clock(enum cg_clk_id id, char *mod_name); */

/* extern int enable_clock_ext_locked(int id, char *mod_name); */
/* extern int disable_clock_ext_locked(int id, char *mod_name); */

/* extern int clock_is_on(int id); */

/* extern int clkmux_sel(int id, unsigned int clksrc, char *name); */
/* extern void enable_mux(int id, char *name); */
/* extern void disable_mux(int id, char *name); */

/* extern void clk_set_force_on(int id); */
/* extern void clk_clr_force_on(int id); */
/* extern int clk_is_force_on(int id); */

/* pll API */
/* extern int enable_pll(int id, char *mod_name); */
/* extern int disable_pll(int id, char *mod_name); */

/* extern int pll_hp_switch_on(int id, int hp_on); */
/* extern int pll_hp_switch_off(int id, int hp_off); */

/* extern int pll_fsel(int id, unsigned int value); */
/* extern int pll_is_on(int id); */

/* subsys API */
/* extern int enable_subsys(int id, char *mod_name); */
/* extern int disable_subsys(int id, char *mod_name); */

/* extern int subsys_is_on(int id); */
/* extern int md_power_on(int id); */
/* extern int md_power_off(int id, unsigned int timeout); */
/* extern int conn_power_on(void); */
/* extern int conn_power_off(void); */

/* other API */

extern void set_mipi26m(int en);
/* extern void set_ada_ssusb_xtal_ck(int en); */

/* const char* grp_get_name(int id); */
/* extern int clkmgr_is_locked(void); */

/* init */
extern int mt_clkmgr_init(void);

/* extern int clk_monitor_0(enum ckmon_sel ckmon, enum monitor_clk_sel_0 sel, int div); */
/* extern int clk_monitor(enum ckmon_sel ckmon, enum monitor_clk_sel sel, int div); */

/* extern void clk_stat_check(int id); */
extern void slp_check_pm_mtcmos_pll(void);

/* sram debug */
extern void aee_rr_rec_clk(int id, u32 val);

#ifdef CONFIG_MMC_MTK
extern void msdc_clk_status(int *status);
#else
void msdc_clk_status(int *status) { *status = 0; }
#endif

#endif
