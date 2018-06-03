/*
 * Copyright (C) 2017 MediaTek Inc.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pinctrl/pinctrl.h>
#include "ufs.h"
#include "ufshcd.h"
#include "unipro.h"
#include "ufs-mtk.h"
#include "ufs-mtk-platform.h"
#include "mtk_idle.h"
#include "mtk_spm_resource_req.h"
#include "mtk_secure_api.h"

#ifdef MTK_UFS_HQA
#include <mtk_reboot.h>
#include <upmu_common.h>
#endif

/* If SPM function not ready, comment this define */
#define SPM_READY /* need platform porting */

/* If CLKBUF function not ready, comment this define */
#define CLKBUF_READY /* need platform porting */

#ifdef CLKBUF_READY
#include "mtk_clkbuf_ctl.h"
#endif

static void __iomem *ufs_mtk_mmio_base_gpio;
static void __iomem *ufs_mtk_mmio_base_infracfg_ao;
static void __iomem *ufs_mtk_mmio_base_pericfg;
static void __iomem *ufs_mtk_mmio_base_ufs_mphy;
static struct regulator *reg_va09;

/**
 * ufs_mtk_pltfrm_pwr_change_final_gear - change pwr mode fianl gear value.
 */
void ufs_mtk_pltfrm_pwr_change_final_gear(struct ufs_hba *hba, struct ufs_pa_layer_attr *final)
{
	/* Change final gear if necessary */
}

#ifdef MTK_UFS_HQA

/* UFS write is performance 200MB/s, 4KB will take around 20us */
/* Set delay before reset to 100us to cover 4KB & >4KB write */

#define UFS_SPOH_USDELAY_BEFORE_RESET (100)

void random_delay(struct ufs_hba *hba)
{
	u32 time = (sched_clock()%UFS_SPOH_USDELAY_BEFORE_RESET);

	udelay(time);
}
void wdt_pmic_full_reset(struct ufs_hba *hba)
{
	/*
	 * Cmd issue to PMIC on MT6771 will take around 20us ~ 30us, in order to speed up VEMC disable time,
	 * we disable VEMC first coz PMIC cold reset may take longer to disable VEMC in it's reset flow.
	 * Can not use regulator_disable() here because it can not use in  preemption disabled context.
	 * Use pmic raw API without nlock instead.
	 */
	pmic_set_register_value_nolock(PMIC_RG_LDO_VEMC_EN, 0);

	/* Need reset external LDO for VUFS18, UFS needs VEMC&VUFS18 reset at the same time */
	pmic_set_register_value_nolock(PMIC_RG_STRUP_EXT_PMIC_SEL, 0x1);

	/* VA09 off, may not require */
	/* pmic_set_register_value_nolock(PMIC_RG_STRUP_EXT_PMIC_EN, 0x1); */
	/* pmic_set_register_value_nolock(PMIC_RG_STRUP_EXT_PMIC_SEL, 0x2); */

	/* PMIC cold reset */
	pmic_set_register_value_nolock(PMIC_RG_CRST, 1);
}
#endif

#include <mt-plat/upmu_common.h>
#define PMIC_REG_MASK (0xFFFF)
#define PMIC_REG_SHIFT (0)
void ufs_mtk_pltfrm_gpio_trigger_and_debugInfo_dump(struct ufs_hba *hba)
{
	int vcc_enabled, vcc_value, vccq2_enabled, va09_enabled;

	vcc_enabled = pmic_get_register_value(PMIC_DA_VEMC_EN);
	vcc_value = pmic_get_register_value(PMIC_RG_VEMC_VOSEL);
	vccq2_enabled = pmic_get_register_value(PMIC_DA_EXT_PMIC_EN1);
	va09_enabled = pmic_get_register_value(PMIC_DA_EXT_PMIC_EN1);
	/* dump vcc, vccq2 and va09 info */
	dev_info(hba->dev, "vcc_enabled:%d, vcc_value:%d, vccq2_enabled:%d, va09:%d!!!\n",
				vcc_enabled, vcc_value, vccq2_enabled, va09_enabled);
	/* dump clock buffer */
	clk_buf_dump_clkbuf_log();
}

#ifdef CONFIG_MTK_UFS_DEGUG_GPIO_TRIGGER
#include <mt-plat/mtk_gpio.h>
#define gpioPin (177UL)
void ufs_mtk_pltfrm_gpio_trigger_init(struct ufs_hba *hba)
{
	/* Need porting for UFS Debug*/
	/* Set gpio mode */
	/* Set gpio output */
	/* Set gpio default low */
	dev_info(hba->dev, "%s: trigger_gpio_init!!!\n",
				__func__);
}
#endif

/*
 * In early-porting stage, because of no bootrom, something finished by bootrom shall be finished here instead.
 * Returns:
 *  0: Successful.
 *  Non-zero: Failed.
 */
int ufs_mtk_pltfrm_bootrom_deputy(struct ufs_hba *hba)
{
#ifdef CONFIG_FPGA_EARLY_PORTING

	u32 reg;

	if (!ufs_mtk_mmio_base_pericfg)
		return 1;

	reg = readl(ufs_mtk_mmio_base_pericfg + REG_UFS_PERICFG);
	reg = reg | (1 << REG_UFS_PERICFG_RST_N_BIT);
	writel(reg, ufs_mtk_mmio_base_pericfg + REG_UFS_PERICFG);

#endif
#ifdef CONFIG_MTK_UFS_DEGUG_GPIO_TRIGGER
	ufs_mtk_pltfrm_gpio_trigger_init(hba);
#endif

	return 0;
}

/**
 * ufs_mtk_deepidle_hibern8_check - callback function for Deepidle & SODI.
 * Release all resources: DRAM/26M clk/Main PLL and dsiable 26M ref clk if in H8.
 *
 * @return: 0 for success, negative/postive error code otherwise
 */
int ufs_mtk_pltfrm_deepidle_check_h8(void)
{
#ifdef SPM_READY
	int ret = 0;
	u32 tmp;

	/**
	 * If current device is not active, it means it is after ufshcd_suspend() through
	 * a. runtime or system pm b. ufshcd_shutdown
	 * Plus deepidle/SODI can not enter in ufs suspend/resume callback by idle_lock_by_ufs()
	 * Therefore, it's guranteed that UFS is in H8 now and 26MHz ref clk is disabled by suspend callback
	 * deepidle/SODI do not need to disable 26MHz ref clk here.
	 * Not use hba->uic_link_state to judge it's after ufshcd_suspend() is because
	 * hba->uic_link_state also used by ufshcd_gate_work()
	 */
	if (ufs_mtk_hba->curr_dev_pwr_mode != UFS_ACTIVE_PWR_MODE) {
		spm_resource_req(SPM_RESOURCE_USER_UFS, SPM_RESOURCE_RELEASE);
		return UFS_H8_SUSPEND;
	}

	/* Release all resources if entering H8 mode */
	ret = ufs_mtk_generic_read_dme(UIC_CMD_DME_GET, VENDOR_POWERSTATE, 0, &tmp, 100);

	if (ret) {
		/* ret == -1 means there is outstanding req/task/uic/pm ongoing, not an error */
		if (ret != -1)
			dev_err(ufs_mtk_hba->dev, "ufshcd_dme_get 0x%x fail, ret = %d!\n", VENDOR_POWERSTATE, ret);
		return ret;
	}

	if (tmp == VENDOR_POWERSTATE_HIBERNATE) {
		/* delay 100us before DeepIdle/SODI disable XO_UFS for Toshiba device */
		if (ufs_mtk_hba->dev_quirks & UFS_DEVICE_QUIRK_DELAY_BEFORE_DISABLE_REF_CLK)
			udelay(100);
		/* Disable MPHY 26MHz ref clock in H8 mode */
		/* SSPM project will disable MPHY 26MHz ref clock in SSPM deepidle/SODI IPI handler*/
	#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	#ifdef CLKBUF_READY
		clk_buf_ctrl(CLK_BUF_UFS, false);
	#endif
	#endif
		spm_resource_req(SPM_RESOURCE_USER_UFS, SPM_RESOURCE_RELEASE);
		return UFS_H8;
	}

	return -1;
#else
	return -1;
#endif
}


/**
 * ufs_mtk_deepidle_leave - callback function for leaving Deepidle & SODI.
 */
void ufs_mtk_pltfrm_deepidle_leave(void)
{
#ifdef CLKBUF_READY
	/* Enable MPHY 26MHz ref clock after leaving deepidle */
	/* SSPM project will enable MPHY 26MHz ref clock in SSPM deepidle/SODI IPI handler*/

#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
	/* If current device is not active, it means it is after ufshcd_suspend() through */
	/* a. runtime or system pm b. ufshcd_shutdown */
	/* And deepidle/SODI can not enter in ufs suspend/resume callback by idle_lock_by_ufs() */
	/* Therefore, it's guranteed that UFS is in H8 now and 26MHz ref clk is disabled by suspend callback */
	/* deepidle/SODI do not need to enable 26MHz ref clk here */
	if (ufs_mtk_hba->curr_dev_pwr_mode != UFS_ACTIVE_PWR_MODE)
		return;

	clk_buf_ctrl(CLK_BUF_UFS, true);
#endif
#endif
}

/**
 * ufs_mtk_deepidle_resource_req - Deepidle & SODI resource request.
 * @hba: per-adapter instance
 * @resource: DRAM/26M clk/MainPLL resources to be claimed. New claim will substitute old claim.
 */
void ufs_mtk_pltfrm_deepidle_resource_req(struct ufs_hba *hba, unsigned int resource)
{
#ifdef SPM_READY
	spm_resource_req(SPM_RESOURCE_USER_UFS, resource);
#endif
}

/**
 * ufs_mtk_pltfrm_deepidle_lock - Deepidle & SODI lock.
 * @hba: per-adapter instance
 * @lock: lock or unlock deepidle & SODI entrance.
 */
void ufs_mtk_pltfrm_deepidle_lock(struct ufs_hba *hba, bool lock)
{
#ifdef SPM_READY
	if (lock)
		idle_lock_by_ufs(1);
	else
		idle_lock_by_ufs(0);
#endif
}

int ufs_mtk_pltfrm_host_sw_rst(struct ufs_hba *hba, u32 target)
{
	u32 reg;

	if (!ufs_mtk_mmio_base_infracfg_ao) {
		dev_info(hba->dev, "ufs_mtk_host_sw_rst: failed, null ufs_mtk_mmio_base_infracfg_ao.\n");
		return 1;
	}

	dev_dbg(hba->dev, "ufs_mtk_host_sw_rst: 0x%x\n", target);

	if (target & SW_RST_TARGET_UFSHCI) {
		/* reset HCI */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UFSHCI_SW_RST_SET);
		reg = reg | (1 << REG_UFSHCI_SW_RST_SET_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UFSHCI_SW_RST_SET);
	}

	if (target & SW_RST_TARGET_UFSCPT) {
		/* reset AES */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UFSCPT_SW_RST_SET);
		reg = reg | (1 << REG_UFSCPT_SW_RST_SET_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UFSCPT_SW_RST_SET);
	}

	if (target & SW_RST_TARGET_UNIPRO) {
		/* reset UniPro */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UNIPRO_SW_RST_SET);
		reg = reg | (1 << REG_UNIPRO_SW_RST_SET_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UNIPRO_SW_RST_SET);
	}

	if (target & SW_RST_TARGET_MPHY) {
		writel((readl(ufs_mtk_mmio_base_ufs_mphy + 0x001C) &
		~(0x01 << 2)) |
		(0x01 << 2),
		(ufs_mtk_mmio_base_ufs_mphy + 0x001C));
	}

	udelay(100);

	if (target & SW_RST_TARGET_UFSHCI) {
		/* clear HCI reset */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UFSHCI_SW_RST_CLR);
		reg = reg | (1 << REG_UFSHCI_SW_RST_CLR_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UFSHCI_SW_RST_CLR);
	}

	if (target & SW_RST_TARGET_UFSCPT) {
		/* clear AES reset */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UFSCPT_SW_RST_CLR);
		reg = reg | (1 << REG_UFSCPT_SW_RST_CLR_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UFSCPT_SW_RST_CLR);
	}

	if (target & SW_RST_TARGET_UNIPRO) {
		/* clear UniPro reset */
		reg = readl(ufs_mtk_mmio_base_infracfg_ao + REG_UNIPRO_SW_RST_CLR);
		reg = reg | (1 << REG_UNIPRO_SW_RST_CLR_BIT);
		writel(reg, ufs_mtk_mmio_base_infracfg_ao + REG_UNIPRO_SW_RST_CLR);
	}

	if (target & SW_RST_TARGET_MPHY) {
		writel((readl(ufs_mtk_mmio_base_ufs_mphy + 0x001C) &
		~(0x01 << 2)) |
		 (0x00 << 2),
		 (ufs_mtk_mmio_base_ufs_mphy + 0x001C));
	}

	udelay(100);

	return 0;
}

int ufs_mtk_pltfrm_init(void)
{
	return 0;
}

int ufs_mtk_pltfrm_parse_dt(struct ufs_hba *hba)
{
	struct device_node *node_gpio;
	struct device_node *node_pericfg;
	struct device_node *node_ufs_mphy;
	int err = 0;

	/* get ufs_mtk_gpio */
	node_gpio = of_find_compatible_node(NULL, NULL, "mediatek,gpio");

	if (node_gpio) {
		ufs_mtk_mmio_base_gpio = of_iomap(node_gpio, 0);

		if (IS_ERR(*(void **)&ufs_mtk_mmio_base_gpio)) {
			err = PTR_ERR(*(void **)&ufs_mtk_mmio_base_gpio);
			dev_err(hba->dev, "error: ufs_mtk_mmio_base_gpio init fail\n");
			ufs_mtk_mmio_base_gpio = NULL;
		}
	} else
		dev_err(hba->dev, "error: node_gpio init fail\n");


	/* get ufs_mtk_mmio_base_pericfg */

	node_pericfg = of_find_compatible_node(NULL, NULL, "mediatek,pericfg");

	if (node_pericfg) {
		ufs_mtk_mmio_base_pericfg = of_iomap(node_pericfg, 0);

		if (IS_ERR(*(void **)&ufs_mtk_mmio_base_pericfg)) {
			err = PTR_ERR(*(void **)&ufs_mtk_mmio_base_pericfg);
			dev_err(hba->dev, "error: mmio_base_pericfg init fail\n");
			ufs_mtk_mmio_base_pericfg = NULL;
		}
	} else
		dev_err(hba->dev, "error: node_pericfg init fail\n");

	/* get ufs_mtk_mmio_base_ufs_mphy */

	node_ufs_mphy = of_find_compatible_node(NULL, NULL, "mediatek,ufs_mphy");

	if (node_ufs_mphy) {
		ufs_mtk_mmio_base_ufs_mphy = of_iomap(node_ufs_mphy, 0);

		if (IS_ERR(*(void **)&ufs_mtk_mmio_base_ufs_mphy)) {
			err = PTR_ERR(*(void **)&ufs_mtk_mmio_base_ufs_mphy);
			dev_err(hba->dev, "error: ufs_mtk_mmio_base_ufs_mphy init fail\n");
			ufs_mtk_mmio_base_ufs_mphy = NULL;
		}
	} else
		dev_err(hba->dev, "error: ufs_mtk_mmio_base_ufs_mphy init fail\n");

	/* get va09 regulator and enable it */
	reg_va09 = regulator_get(hba->dev, "va09");
	if (!reg_va09) {
		dev_info(hba->dev, "%s: get va09 fail!!!\n", __func__);
		return -EINVAL;
	}
	err = regulator_enable(reg_va09);
	if (err < 0) {
		dev_info(hba->dev, "%s: enalbe va09 fail, err = %d\n", __func__, err);
		return err;
	}

	return err;
}

int ufs_mtk_pltfrm_res_req(struct ufs_hba *hba, u32 option)
{
#ifdef SPM_READY
	if (option == UFS_MTK_RESREQ_DMA_OP) {

		/* request resource for DMA operations, e.g., DRAM */

		ufshcd_vops_deepidle_resource_req(hba,
		  SPM_RESOURCE_MAINPLL | SPM_RESOURCE_DRAM | SPM_RESOURCE_CK_26M);

	} else if (option == UFS_MTK_RESREQ_MPHY_NON_H8) {

		/* request resource for mphy not in H8, e.g., main PLL, 26 mhz clock */

		ufshcd_vops_deepidle_resource_req(hba,
		  SPM_RESOURCE_MAINPLL | SPM_RESOURCE_CK_26M);
	}
#endif
	return 0;
}

int ufs_mtk_pltfrm_resume(struct ufs_hba *hba)
{
	int ret = 0;
	u32 reg;

#ifdef CLKBUF_READY
	/* Enable MPHY 26MHz ref clock */
	clk_buf_ctrl(CLK_BUF_UFS, true);
#endif
	/* Set regulator to turn on VA09 LDO */
	ret = regulator_enable(reg_va09);
	if (ret < 0) {
		dev_info(hba->dev, "%s: enalbe va09 fail, err = %d\n", __func__, ret);
		return ret;
	}

	/* wait 200 us to stablize VA09 */
	udelay(200);

	/* Step 1: Set RG_VA09_ON to 1 */
	mt_secure_call(MTK_SIP_KERNEL_UFS_CTL, 1, 1, 0);

	/* Step 2: release DA_MP_PLL_PWR_ON */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg | (0x1 << 11);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg & ~(0x1 << 10);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);

	/* Step 3: release DA_MP_PLL_ISO_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg & ~(0x1 << 9);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg & ~(0x1 << 8);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);

	/* Step 4: release DA_MP_CDR_PWR_ON */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg | (0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg & ~(0x1 << 17);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);

	/* Step 5: release DA_MP_CDR_ISO_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg & ~(0x1 << 20);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg & ~(0x1 << 19);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);

	/* Step 6: release DA_MP_RX0_SQ_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = reg | (0x1 << 1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = reg & ~(0x1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);

	/* Delay 1us to wait DIFZ stable */
	udelay(1);

	/* Step 7: release DIFZ */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA09C);
	reg = reg & ~(0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA09C);

	return ret;
}

int ufs_mtk_pltfrm_suspend(struct ufs_hba *hba)
{
	int ret = 0;
	u32 reg;

	/* Step 1: force DIFZ */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA09C);
	reg = reg | (0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA09C);

	/* Step 2: force DA_MP_RX0_SQ_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = reg | (0x1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);
	reg = reg & ~(0x1 << 1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA0AC);

	/* Step 3: force DA_MP_CDR_ISO_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg | (0x1 << 19);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg | (0x1 << 20);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);

	/* Step 4: force DA_MP_CDR_PWR_ON */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg | (0x1 << 17);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA044);
	reg = reg & ~(0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA044);

	/* Step 5: force DA_MP_PLL_ISO_EN */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg | (0x1 << 8);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg | (0x1 << 9);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);

	/* Step 6: force DA_MP_PLL_PWR_ON */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg | (0x1 << 10);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xA08C);
	reg = reg & ~(0x1 << 11);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xA08C);

	/* Step 7: Set RG_VA09_ON to 0 */
	mt_secure_call(MTK_SIP_KERNEL_UFS_CTL, 1, 0, 0);

	/* delay awhile to satisfy T_HIBERNATE */
	mdelay(15);

#ifdef CLKBUF_READY
	/* Disable MPHY 26MHz ref clock in H8 mode */
	clk_buf_ctrl(CLK_BUF_UFS, false);
#endif
	/* Set regulator to turn off VA09 LDO */
	ret = regulator_disable(reg_va09);
	if (ret < 0) {
		dev_info(hba->dev, "%s: disalbe va09 fail, err = %d\n", __func__, ret);
		return ret;
	}
#if 0
	/* TEST ONLY: emulate UFSHCI power off by HCI SW reset */
	ufs_mtk_pltfrm_host_sw_rst(hba, SW_RST_TARGET_UFSHCI);
#endif

	return ret;
}


