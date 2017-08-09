#ifndef _MSDC_IO_H_
#define _MSDC_IO_H_

/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

extern const struct of_device_id msdc_of_ids[];
extern struct msdc_hw *p_msdc_hw[];

extern struct device_node *eint_node;

extern void __iomem *gpio_base;
extern void __iomem *msdc0_io_cfg_base;
extern void __iomem *msdc1_io_cfg_base;
extern void __iomem *msdc2_io_cfg_base;
extern void __iomem *msdc3_io_cfg_base;

extern void __iomem *infracfg_ao_reg_base;
/*
extern void __iomem *infracfg_reg_base;
*/
extern void __iomem *pericfg_reg_base;
/*
extern void __iomem *emi_reg_base;
extern void __iomem *toprgu_reg_base;
*/
extern void __iomem *apmixed_reg_base;
extern void __iomem *topckgen_reg_base;


int msdc_dt_init(struct platform_device *pdev, struct mmc_host *mmc,
		unsigned int *cd_irq);

#ifdef FPGA_PLATFORM
void msdc_fpga_pwr_init(void);
#endif

/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#include <mt-plat/upmu_common.h>

#define REG_VEMC_VOSEL_CAL              MT6351_PMIC_RG_VEMC_CAL_ADDR
#define REG_VEMC_VOSEL                  MT6351_PMIC_RG_VEMC_VOSEL_ADDR
#define REG_VEMC_EN                     MT6351_PMIC_RG_VEMC_EN_ADDR
#define REG_VMC_VOSEL                   MT6351_PMIC_RG_VMC_VOSEL_ADDR
#define REG_VMC_EN                      MT6351_PMIC_RG_VMC_EN_ADDR
#define REG_VMCH_VOSEL_CAL              MT6351_PMIC_RG_VMCH_CAL_ADDR
#define REG_VMCH_VOSEL                  MT6351_PMIC_RG_VMCH_VOSEL_ADDR
#define REG_VMCH_EN                     MT6351_PMIC_RG_VMCH_EN_ADDR

#define MASK_VEMC_VOSEL_CAL             MT6351_PMIC_RG_VEMC_CAL_MASK
#define SHIFT_VEMC_VOSEL_CAL            MT6351_PMIC_RG_VEMC_CAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL            (MASK_VEMC_VOSEL_CAL << \
						SHIFT_VEMC_VOSEL_CAL)
#define MASK_VEMC_VOSEL                 MT6351_PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL                MT6351_PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL                (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)
#define MASK_VEMC_EN                    MT6351_PMIC_RG_VEMC_EN_MASK
#define SHIFT_VEMC_EN                   MT6351_PMIC_RG_VEMC_EN_SHIFT
#define FIELD_VEMC_EN                   (MASK_VEMC_EN << SHIFT_VEMC_EN)
#define MASK_VMC_VOSEL                  MT6351_PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL                 MT6351_PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL                 (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)
#define MASK_VMC_EN                     MT6351_PMIC_RG_VMC_EN_MASK
#define SHIFT_VMC_EN                    MT6351_PMIC_RG_VMC_EN_SHIFT
#define FIELD_VMC_EN                    (MASK_VMC_EN << SHIFT_VMC_EN)
#define MASK_VMCH_VOSEL_CAL             MT6351_PMIC_RG_VMCH_CAL_MASK
#define SHIFT_VMCH_VOSEL_CAL            MT6351_PMIC_RG_VMCH_CAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL            (MASK_VMCH_VOSEL_CAL << \
						SHIFT_VMCH_VOSEL_CAL)
#define MASK_VMCH_VOSEL                 MT6351_PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL                MT6351_PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL                (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)
#define MASK_VMCH_EN                    MT6351_PMIC_RG_VMCH_EN_MASK
#define SHIFT_VMCH_EN                   MT6351_PMIC_RG_VMCH_EN_SHIFT
#define FIELD_VMCH_EN                   (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define VEMC_VOSEL_CAL_mV(cal)          ((cal <= 0) ? ((0-cal)/20) : (32-cal/20))
#define VEMC_VOSEL_3V                   (0)
#define VEMC_VOSEL_3V3                  (1)
#define VMC_VOSEL_1V8                   (3)
#define VMC_VOSEL_2V8                   (5)
#define VMC_VOSEL_3V                    (6)
#define VMCH_VOSEL_CAL_mV(cal)          ((cal <= 0) ? ((0-cal)/20) : (32-cal/20))
#define VMCH_VOSEL_3V                   (0)
#define VMCH_VOSEL_3V3                  (1)

#define REG_VCORE_VOSEL_SW		MT6351_PMIC_BUCK_VCORE_VOSEL_ADDR
#define	VCORE_VOSEL_SW_MASK		MT6351_PMIC_BUCK_VCORE_VOSEL_MASK
#define	VCORE_VOSEL_SW_SHIFT		MT6351_PMIC_BUCK_VCORE_VOSEL_SHIFT
#define REG_VCORE_VOSEL_HW		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_ADDR
#define	VCORE_VOSEL_HW_MASK		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_MASK
#define	VCORE_VOSEL_HW_SHIFT		MT6351_PMIC_BUCK_VCORE_VOSEL_ON_SHIFT
#define REG_VIO18_CAL			MT6351_PMIC_RG_VIO18_CAL_ADDR
#define VIO18_CAL_MASK			MT6351_PMIC_RG_VIO18_CAL_MASK
#define VIO18_CAL_SHIFT			MT6351_PMIC_RG_VIO18_CAL_SHIFT

#define CARD_VOL_ACTUAL			VOL_2900

void msdc_sd_power_switch(struct msdc_host *host, u32 on);

#if !defined(FPGA_PLATFORM)
void msdc_emmc_power(struct msdc_host *host, u32 on);
void msdc_sd_power(struct msdc_host *host, u32 on);
void msdc_sdio_power(struct msdc_host *host, u32 on);
void msdc_dump_ldo_sts(struct msdc_host *host);
#endif

#ifdef FPGA_PLATFORM
void msdc_fpga_power(struct msdc_host *host, u32 on);
bool hwPowerOn_fpga(void);
bool hwPowerDown_fpga(void);
bool hwPowerDown_fpga(void);
#define msdc_emmc_power			msdc_fpga_power
#define msdc_sd_power			msdc_fpga_power
#define msdc_sdio_power			msdc_fpga_power
#define msdc_dump_ldo_sts(host)
#endif

extern u32 g_msdc0_io;
extern u32 g_msdc0_flash;
extern u32 g_msdc1_io;
extern u32 g_msdc1_flash;
extern u32 g_msdc2_io;
extern u32 g_msdc2_flash;
extern u32 g_msdc3_io;
extern u32 g_msdc3_flash;

/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if defined(FPGA_PLATFORM)
#define msdc_dump_clock_sts(host)
extern u32 hclks_msdc[];
#define msdc_get_hclks(host)		hclks_msdc
#define msdc_clk_enable(host)
#define msdc_clk_disable(host)
#endif

#if !defined(FPGA_PLATFORM)
void msdc_dump_clock_sts(struct msdc_host *host);
/* define clock related register macro */
#define MSDCPLL_CON0_OFFSET             (0x240)
#define MSDCPLL_CON1_OFFSET             (0x244)
#define MSDCPLL_PWR_CON0_OFFSET         (0x24C)
#define MSDC_CLK_CFG_2_OFFSET           (0x060)
#define MSDC_CLK_CFG_3_OFFSET           (0x070)

#define MSDC_PERI_PDN_SET0_OFFSET       (0x0008)
#define MSDC_PERI_PDN_CLR0_OFFSET       (0x0010)
#define MSDC_PERI_PDN_STA0_OFFSET       (0x0018)

extern u32 hclks_msdc50[];
extern u32 hclks_msdc30[];
extern u32 hclks_msdc30_3[];
#define msdc_get_hclks(id) \
	((id == 0) ? hclks_msdc50 : ((id == 3) ? hclks_msdc30_3 : hclks_msdc30))

#if defined(CONFIG_MTK_LEGACY)
extern enum cg_clk_id msdc_cg_clk_id[];
#define msdc_clk_enable(host) \
	enable_clock(msdc_cg_clk_id[host->id], "SD")
#define msdc_clk_disable(host)	\
	disable_clock(msdc_cg_clk_id[host->id], "SD")
#endif

#if !defined(CONFIG_MTK_LEGACY)
#define msdc_clk_enable(host) clk_disable(host->clock_control)
#define msdc_clk_disable(host) clk_enable(host->clock_control)
#endif

#endif

/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*******************************************************************************
 *PINMUX and GPIO definition
 ******************************************************************************/

#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

/* add pull down/up mode define */
#define MSDC_GPIO_PULL_UP       (0)
#define MSDC_GPIO_PULL_DOWN     (1)

/*--------------------------------------------------------------------------*/
/* MSDC0~3 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE		gpio_base
#define MSDC0_IO_PAD_BASE	(msdc0_io_cfg_base)
#define MSDC1_IO_PAD_BASE	(msdc1_io_cfg_base)
#define MSDC2_IO_PAD_BASE	(msdc2_io_cfg_base)
#define MSDC3_IO_PAD_BASE	(msdc3_io_cfg_base)

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
#define MSDC0_GPIO_MODE18	(MSDC_GPIO_BASE + 0x410)
#define MSDC0_GPIO_MODE19	(MSDC_GPIO_BASE + 0x420)
#define MSDC0_GPIO_IES_ADDR	(MSDC0_IO_PAD_BASE + 0x000)
#define MSDC0_GPIO_SMT_ADDR	(MSDC0_IO_PAD_BASE + 0x010)
#define MSDC0_GPIO_TDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x020)
#define MSDC0_GPIO_RDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x040)
#define MSDC0_GPIO_SR_ADDR	(MSDC0_IO_PAD_BASE + 0x0a0)
#define MSDC0_GPIO_DRV_ADDR	(MSDC0_IO_PAD_BASE + 0x0a0)
#define MSDC0_GPIO_PUPD0_ADDR	(MSDC0_IO_PAD_BASE + 0x0c0)
#define MSDC0_GPIO_PUPD1_ADDR	(MSDC0_IO_PAD_BASE + 0x0d0)

#define MSDC1_GPIO_MODE4	(MSDC_GPIO_BASE + 0x330)
#define MSDC1_GPIO_IES_ADDR	(MSDC1_IO_PAD_BASE + 0x000)
#define MSDC1_GPIO_SMT_ADDR	(MSDC1_IO_PAD_BASE + 0x010)
#define MSDC1_GPIO_TDSEL_ADDR	(MSDC1_IO_PAD_BASE + 0x030)
#define MSDC1_GPIO_RDSEL0_ADDR	(MSDC1_IO_PAD_BASE + 0x040)
#define MSDC1_GPIO_RDSEL1_ADDR	(MSDC1_IO_PAD_BASE + 0x050)
#define MSDC1_GPIO_SR_ADDR	(MSDC1_IO_PAD_BASE + 0x0b0)
#define MSDC1_GPIO_DRV_ADDR	(MSDC1_IO_PAD_BASE + 0x0b0)
#define MSDC1_GPIO_PUPD_ADDR	(MSDC1_IO_PAD_BASE + 0x0c0)

#define MSDC2_GPIO_MODE14	(MSDC_GPIO_BASE + 0x3D0)
#define MSDC2_GPIO_IES_ADDR	(MSDC2_IO_PAD_BASE + 0x000)
#define MSDC2_GPIO_SMT_ADDR	(MSDC2_IO_PAD_BASE + 0x010)
#define MSDC2_GPIO_TDSEL_ADDR	(MSDC2_IO_PAD_BASE + 0x020)
#define MSDC2_GPIO_RDSEL_ADDR	(MSDC2_IO_PAD_BASE + 0x040)
#define MSDC2_GPIO_SR_ADDR	(MSDC2_IO_PAD_BASE + 0x0a0)
#define MSDC2_GPIO_DRV_ADDR	(MSDC2_IO_PAD_BASE + 0x0a0)
#define MSDC2_GPIO_PUPD_ADDR	(MSDC2_IO_PAD_BASE + 0x0c0)

/*
 * MSDC0 GPIO and PAD register and bitfields definition
 */
/*MSDC0_GPIO_MODE19, Set as pinmux value as 1*/
#define MSDC0_MODE_DAT7_MASK            (7 << 25)
#define MSDC0_MODE_DAT6_MASK            (7 <<  9)
#define MSDC0_MODE_DAT5_MASK            (7 <<  6)
#define MSDC0_MODE_DAT4_MASK            (7 << 12)
#define MSDC0_MODE_DAT3_MASK            (7 << 22)
#define MSDC0_MODE_DAT2_MASK            (7 << 19)
#define MSDC0_MODE_DAT1_MASK            (7 <<  3)
#define MSDC0_MODE_CMD_MASK             (7 <<  0)
#define MSDC0_MODE_DSL_MASK             (7 << 28)
#define MSDC0_MODE_RST_MASK             (7 << 16)

/*MSDC0_GPIO_MODE18, Set as pinmux value as 1*/
#define MSDC0_MODE_DAT0_MASK            (7 << 25)
#define MSDC0_MODE_CLK_MASK             (7 << 28)

/* MSDC0 IES mask*/
#define MSDC0_IES_DAT_MASK              (0x1  <<  0)
#define MSDC0_IES_CLK_MASK              (0x1  <<  1)
#define MSDC0_IES_CMD_MASK              (0x1  <<  2)
#define MSDC0_IES_RSTB_MASK             (0x1  <<  3)
#define MSDC0_IES_DSL_MASK              (0x1  <<  4)
#define MSDC0_IES_ALL_MASK              (0x1F <<  0)

/* MSDC0 SMT mask*/
#define MSDC0_SMT_DAT_MASK              (0x1  <<  0)
#define MSDC0_SMT_CLK_MASK              (0x1  <<  1)
#define MSDC0_SMT_CMD_MASK              (0x1  <<  2)
#define MSDC0_SMT_RSTB_MASK             (0x1  <<  3)
#define MSDC0_SMT_DSL_MASK              (0x1  <<  4)
#define MSDC0_SMT_ALL_MASK              (0x1F <<  0)

/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_DAT_MASK            (0xF  <<  0)
#define MSDC0_TDSEL_CLK_MASK            (0xF  <<  4)
#define MSDC0_TDSEL_CMD_MASK            (0xF  <<  8)
#define MSDC0_TDSEL_RSTB_MASK           (0xF  << 12)
#define MSDC0_TDSEL_DSL_MASK            (0xF  << 16)
#define MSDC0_TDSEL_ALL_MASK            (0xFFFFF << 0)

/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_DAT_MASK            (0x3F <<  0)
#define MSDC0_RDSEL_CLK_MASK            (0x3F <<  6)
#define MSDC0_RDSEL_CMD_MASK            (0x3F << 12)
#define MSDC0_RDSEL_RSTB_MASK           (0x3F << 18)
#define MSDC0_RDSEL_DSL_MASK            (0x3F << 24)
#define MSDC0_RDSEL_ALL_MASK            (0x3FFFFFFF << 0)

/* MSDC0 SR mask*/
#define MSDC0_SR_DAT_MASK               (0x1  <<  3)
#define MSDC0_SR_CLK_MASK               (0x1  <<  7)
#define MSDC0_SR_CMD_MASK               (0x1  << 11)
#define MSDC0_SR_RSTB_MASK              (0x1  << 15)
#define MSDC0_SR_DSL_MASK               (0x1  << 19)
/*Attention: bits are not continuous, shall not define MSDC0_SR_ALL_MASK*/

/* MSDC0 DRV mask*/
#define MSDC0_DRV_DAT_MASK              (0x7  <<  0)
#define MSDC0_DRV_CLK_MASK              (0x7  <<  4)
#define MSDC0_DRV_CMD_MASK              (0x7  <<  8)
#define MSDC0_DRV_RSTB_MASK             (0x7  << 12)
#define MSDC0_DRV_DSL_MASK              (0x7  << 16)
/*Attention: bits are not continuous, shall not define MSDC0_DRV_ALL_MASK*/

/* MSDC0 PUPD mask*/
#define MSDC0_PUPD_DAT0_MASK            (0x7  <<  0)
#define MSDC0_PUPD_CLK_MASK             (0x7  <<  4)
#define MSDC0_PUPD_CMD_MASK             (0x7  <<  8)
#define MSDC0_PUPD_DAT1_MASK            (0x7  << 12)
#define MSDC0_PUPD_DAT5_MASK            (0x7  << 16)
#define MSDC0_PUPD_DAT6_MASK            (0x7  << 20)
#define MSDC0_PUPD_DAT4_MASK            (0x7  << 24)
#define MSDC0_PUPD_RSTB_MASK            (0x7  << 28)
#define MSDC0_PUPD0_MASK_WITH_RSTB      (0x77777777 << 0)
#define MSDC0_PUPD0_MASK                (0x7777777 << 0)

#define MSDC0_PUPD_DAT2_MASK            (0x7  <<  0)
#define MSDC0_PUPD_DAT3_MASK            (0x7  <<  4)
#define MSDC0_PUPD_DAT7_MASK            (0x7  <<  8)
#define MSDC0_PUPD_DSL_MASK             (0x7  << 12)
#define MSDC0_PUPD1_MASK                (0x7777 << 0)

/*
 * MSDC1 GPIO and PAD register and bitfields definition
 */
/*MSDC0_GPIO_MODE4, Set as pinmux value as 1*/
#define MSDC1_MODE_CLK_MASK             (7 <<  0)
#define MSDC1_MODE_CMD_MASK             (7 <<  6)
#define MSDC1_MODE_DAT0_MASK            (7 <<  9)
#define MSDC1_MODE_DAT1_MASK            (7 << 16)
#define MSDC1_MODE_DAT2_MASK            (7 << 12)
#define MSDC1_MODE_DAT3_MASK            (7 <<  3)

/* MSDC1 IES mask*/
#define MSDC1_IES_CLK_MASK              (0x1 <<  8)
#define MSDC1_IES_CMD_MASK              (0x1 <<  9)
#define MSDC1_IES_DAT_MASK              (0x1 << 10)
#define MSDC1_IES_ALL_MASK              (0x7 <<  8)

/* MSDC1 SMT mask*/
#define MSDC1_SMT_CLK_MASK              (0x1 <<  8)
#define MSDC1_SMT_CMD_MASK              (0x1 <<  9)
#define MSDC1_SMT_DAT_MASK              (0x1 << 10)
#define MSDC1_SMT_ALL_MASK              (0x7 <<  8)

/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_CLK_MASK            (0xF <<  0)
#define MSDC1_TDSEL_CMD_MASK            (0xF <<  4)
#define MSDC1_TDSEL_DAT_MASK            (0xF <<  8)
#define MSDC1_TDSEL_ALL_MASK            (0xFFF << 0)

/* MSDC1 RDSEL mask*/
/*MSDC1_GPIO_RDSEL0*/
#define MSDC1_RDSEL_CLK_MASK            (0x3F << 16)
#define MSDC1_RDSEL_CMD_MASK            (0x3F << 22)
/*MSDC1_GPIO_RDSEL1*/
#define MSDC1_RDSEL_DAT_MASK            (0x3F <<  0)
#define MSDC1_RDSEL0_ALL_MASK           (0xFFF << 16)
#define MSDC1_RDSEL1_ALL_MASK           (0x3F <<  0)
/*Attention: not the same address, shall not define MSDC1_RDSEL_ALL_MASK*/

/* MSDC1 SR mask*/
#define MSDC1_SR_CLK_MASK               (0x1 <<  3)
#define MSDC1_SR_CMD_MASK               (0x1 <<  7)
#define MSDC1_SR_DAT_MASK               (0x1 << 11)
/*Attention: bits are not continuous, shall not define MSDC1_SR_ALL_MASK*/

/* MSDC1 DRV mask*/
#define MSDC1_DRV_CLK_MASK              (0x7 <<  0)
#define MSDC1_DRV_CMD_MASK              (0x7 <<  4)
#define MSDC1_DRV_DAT_MASK              (0x7 <<  8)
/*Attention: bits are not continuous, shall not define MSDC1_DRV_ALL_MASK*/

/* MSDC1 PUPD mask*/
#define MSDC1_PUPD_CMD_MASK             (0x7  <<  0)
#define MSDC1_PUPD_CLK_MASK             (0x7  <<  4)
#define MSDC1_PUPD_DAT0_MASK            (0x7  <<  8)
#define MSDC1_PUPD_DAT1_MASK            (0x7  << 12)
#define MSDC1_PUPD_DAT2_MASK            (0x7  << 16)
#define MSDC1_PUPD_DAT3_MASK            (0x7  << 20)
#define MSDC1_PUPD_MASK                 (0x777777 << 0)

/*
 * MSDC2 GPIO and PAD register and bitfields definition
 */
/*MSDC2_GPIO_MODE14, Set as pinmux value as 2*/
#define MSDC2_MODE_CLK_MASK             (7 <<  9)
#define MSDC2_MODE_CMD_MASK             (7 <<  6)
#define MSDC2_MODE_DAT0_MASK            (7 << 16)
#define MSDC2_MODE_DAT1_MASK            (7 <<  3)
#define MSDC2_MODE_DAT2_MASK            (7 << 19)
#define MSDC2_MODE_DAT3_MASK            (7 << 12)

/* MSDC2 IES mask*/
#define MSDC2_IES_CLK_MASK              (0x1 << 5)
#define MSDC2_IES_CMD_MASK              (0x1 << 4)
#define MSDC2_IES_DAT_MASK              (0x1 << 3)
#define MSDC2_IES_ALL_MASK              (0x7 << 3)

/* MSDC2 SMT mask*/
#define MSDC2_SMT_CLK_MASK              (0x1 << 5)
#define MSDC2_SMT_CMD_MASK              (0x1 << 4)
#define MSDC2_SMT_DAT_MASK              (0x1 << 3)
#define MSDC2_SMT_ALL_MASK              (0x7 << 3)

/* MSDC2 TDSEL mask*/
#define MSDC2_TDSEL_CLK_MASK            (0xF << 20)
#define MSDC2_TDSEL_CMD_MASK            (0xF << 16)
#define MSDC2_TDSEL_DAT_MASK            (0xF << 12)
#define MSDC2_TDSEL_ALL_MASK            (0xFFF << 12)

/* MSDC2 RDSEL mask*/
#define MSDC2_RDSEL_CLK_MASK            (0x3F << 22)
#define MSDC2_RDSEL_CMD_MASK            (0x3F << 16)
#define MSDC2_RDSEL_DAT_MASK            (0x3F << 10)
#define MSDC2_RDSEL_ALL_MASK            (0x3FFFF << 10)

/* MSDC2 SR mask*/
#define MSDC2_SR_CLK_MASK               (0x1 << 23)
#define MSDC2_SR_CMD_MASK               (0x1 << 19)
#define MSDC2_SR_DAT_MASK               (0x1 << 15)

/* MSDC2 DRV mask*/
#define MSDC2_DRV_CLK_MASK              (0x7 << 20)
#define MSDC2_DRV_CMD_MASK              (0x7 << 16)
#define MSDC2_DRV_DAT_MASK              (0x7 << 12)

/* MSDC2 PUPD mask*/
#define MSDC2_PUPD_CLK_MASK             (0x7 << 20)
#define MSDC2_PUPD_CMD_MASK             (0x7 << 16)
#define MSDC2_PUPD_DAT_MASK             (0x7 << 12)
#define MSDC2_PUPD_MASK                 (0x777 << 0)

#ifndef FPGA_PLATFORM
void msdc_set_driving_by_id(u32 id, struct msdc_hw *hw, bool sd_18);
void msdc_get_driving_by_id(u32 id, struct msdc_ioctl *msdc_ctl);
void msdc_set_ies_by_id(u32 id, int set_ies);
void msdc_set_sr_by_id(u32 id, int clk, int cmd, int dat, int rst, int ds);
void msdc_set_smt_by_id(u32 id, int set_smt);
void msdc_set_tdsel_by_id(u32 id, bool sleep, bool sd_18);
void msdc_set_rdsel_by_id(u32 id, bool sleep, bool sd_18);
void msdc_set_tdsel_dbg_by_id(u32 id, u32 value);
void msdc_set_rdsel_dbg_by_id(u32 id, u32 value);
void msdc_get_tdsel_dbg_by_id(u32 id, u32 *value);
void msdc_get_rdsel_dbg_by_id(u32 id, u32 *value);
void msdc_dump_padctl_by_id(u32 id);
void msdc_pin_config_by_id(u32 id, u32 mode);
#endif

#ifdef FPGA_PLATFORM
#define msdc_set_driving_by_id(id, hw, sd_18)
#define msdc_get_driving_by_id(host, msdc_ctl)
#define msdc_set_ies_by_id(id, set_ies)
#define msdc_set_sr_by_id(id, clk, cmd, dat, rst, ds)
#define msdc_set_smt_by_id(id, set_smt)
#define msdc_set_tdsel_by_id(id, sleep, sd_18)
#define msdc_set_rdsel_by_id(id, sleep, sd_18)
#define msdc_set_tdsel_dbg_by_id(id, value)
#define msdc_set_rdsel_dbg_by_id(id, value)
#define msdc_get_tdsel_dbg_by_id(id, value)
#define msdc_get_rdsel_dbg_by_id(id, value)
#define msdc_dump_padctl_by_id(id)
#define msdc_pin_config_by_id(id, mode)
#define msdc_set_pin_mode(host)
#endif

#define msdc_get_driving(host, msdc_ctl) \
	msdc_get_driving_by_id(host->id, msdc_ctl)

#define msdc_set_driving(host, hw, sd_18) \
	msdc_set_driving_by_id(host->id, hw, sd_18)

#define msdc_set_ies(host, set_ies) \
	msdc_set_ies_by_id(host->id, set_ies)

#define msdc_set_sr(host, clk, cmd, dat, rst, ds) \
	msdc_set_sr_by_id(host->id, clk, cmd, dat, rst, ds)

#define msdc_set_smt(host, set_smt) \
	msdc_set_smt_by_id(host->id, set_smt)

#define msdc_set_tdsel(host, sleep, sd_18) \
	msdc_set_tdsel_by_id(host->id, sleep, sd_18)

#define msdc_set_rdsel(host, sleep, sd_18) \
	msdc_set_rdsel_by_id(host->id, sleep, sd_18)

#define msdc_set_tdsel_dbg(host, value) \
	msdc_set_tdsel_dbg_by_id(host->id, value)

#define msdc_set_rdsel_dbg(host, value) \
	msdc_set_rdsel_dbg_by_id(host->id, value)

#define msdc_get_tdsel_dbg(host, value) \
	msdc_get_tdsel_dbg_by_id(host->id, value)

#define msdc_get_rdsel_dbg(host, value) \
	msdc_get_rdsel_dbg_by_id(host->id, value)

#define msdc_dump_padctl(host) \
	msdc_dump_padctl_by_id(host->id)

#define msdc_pin_config(host, mode) \
	msdc_pin_config_by_id(host->id, mode)

/**************************************************************/
/* Section 5: MISC                                            */
/**************************************************************/
void dump_axi_bus_info(void);
void dump_emi_info(void);
void msdc_polling_axi_status(int line, int dead);

#endif /* end of _MSDC_IO_H_ */
