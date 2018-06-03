/*
* Copyright (C) 2016 MediaTek Inc.
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
#include "mtk_clkbuf_ctl.h"

#ifdef MTK_UFS_HQA
#include <mtk_reboot.h>
#include <upmu_common.h>
#endif

static void __iomem *ufs_mtk_mmio_base_gpio;
static void __iomem *ufs_mtk_mmio_base_infracfg_ao;
static void __iomem *ufs_mtk_mmio_base_pericfg;
static void __iomem *ufs_mtk_mmio_base_ufs_mphy;
static struct pinctrl *ufs_mtk_pinctrl;
static struct pinctrl_state *ufs_mtk_pins_default;
static struct pinctrl_state *ufs_mtk_pins_va09_on;
static struct pinctrl_state *ufs_mtk_pins_va09_off;

/**
 * ufs_mtk_pltfrm_pwr_change_final_gear - change pwr mode fianl gear value.
 */
void ufs_mtk_pltfrm_pwr_change_final_gear(struct ufs_hba *hba, struct ufs_pa_layer_attr *final)
{
	/* Change final gear if necessary */
}


#ifdef MTK_UFS_HQA

/* UFS write is performance  200MB/s, 4KB will take around 20us */
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
	  *   Cmd issue to PMIC on MT6763 will take around 20us ~ 30us, in order to speed up VEMC disable time,
	  * we disable VEMC first coz PMIC cold reset may take longer to disable VEMC in it's reset flow.
	  * Can not use regulator_disable() here because it can not use in  preemption disabled context.
	  * Use pmic raw API without nlock instead.
	  */
	pmic_set_register_value_nolock(PMIC_RG_LDO_VEMC_EN, 0);
	/* PMIC cold reset */
	pmic_set_register_value_nolock(PMIC_RG_CRST, 1);
}
#endif

#include <mt-plat/upmu_common.h>
#define PMIC_REG_MASK (0xFFFF)
#define PMIC_REG_SHIFT (0)
void ufs_mtk_pltfrm_gpio_trigger_and_debugInfo_dump(struct ufs_hba *hba)
{
	/* Need porting for UFS Debug */
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
	dev_err(hba->dev, "%s: trigger_gpio_init!!!\n",
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
		spm_resource_req(SPM_RESOURCE_USER_UFS, 0);
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
		/* Disable MPHY 26MHz ref clock in H8 mode */
		/* SSPM project will disable MPHY 26MHz ref clock in SSPM deepidle/SODI IPI handler*/
	#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
		clk_buf_ctrl(CLK_BUF_UFS, false);
	#endif
		spm_resource_req(SPM_RESOURCE_USER_UFS, 0);
		return UFS_H8;
	}

	return -1;
}


/**
 * ufs_mtk_deepidle_leave - callback function for leaving Deepidle & SODI.
 */
void ufs_mtk_pltfrm_deepidle_leave(void)
{
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
}

/**
 * ufs_mtk_deepidle_resource_req - Deepidle & SODI resource request.
 * @hba: per-adapter instance
 * @resource: DRAM/26M clk/MainPLL resources to be claimed. New claim will substitute old claim.
 */
void ufs_mtk_pltfrm_deepidle_resource_req(struct ufs_hba *hba, unsigned int resource)
{
	spm_resource_req(SPM_RESOURCE_USER_UFS, resource);
}

/**
 * ufs_mtk_pltfrm_deepidle_lock - Deepidle & SODI lock.
 * @hba: per-adapter instance
 * @lock: lock or unlock deepidle & SODI entrance.
 */
void ufs_mtk_pltfrm_deepidle_lock(struct ufs_hba *hba, bool lock)
{
	if (lock)
		idle_lock_by_ufs(1);
	else
		idle_lock_by_ufs(0);
}

int ufs_mtk_pltfrm_host_sw_rst(struct ufs_hba *hba, u32 target)
{
	u32 reg;

	if (!ufs_mtk_mmio_base_infracfg_ao) {
		dev_err(hba->dev, "ufs_mtk_host_sw_rst: failed, null ufs_mtk_mmio_base_infracfg_ao.\n");
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

	/* get and configure pinctrl */

	ufs_mtk_pinctrl = devm_pinctrl_get(hba->dev);

	if (IS_ERR(ufs_mtk_pinctrl)) {
		err = PTR_ERR(ufs_mtk_pinctrl);
		dev_err(hba->dev, "error: ufs_mtk_pinctrl init fail\n");
		return err;
	}

	ufs_mtk_pins_default = pinctrl_lookup_state(ufs_mtk_pinctrl, "default");

	if (IS_ERR(ufs_mtk_pins_default)) {
		err = PTR_ERR(ufs_mtk_pins_default);
		dev_err(hba->dev, "error: ufs_mtk_pins_default init fail\n");
		return err;
	}

	ufs_mtk_pins_va09_on = pinctrl_lookup_state(ufs_mtk_pinctrl, "state_va09_on");

	if (IS_ERR(ufs_mtk_pins_va09_on)) {
		err = PTR_ERR(ufs_mtk_pins_va09_on);
		ufs_mtk_pins_va09_on = NULL;
		dev_err(hba->dev, "error: ufs_mtk_pins_va09_on init fail\n");
		return err;
	}

	ufs_mtk_pins_va09_off = pinctrl_lookup_state(ufs_mtk_pinctrl, "state_va09_off");

	if (IS_ERR(ufs_mtk_pins_va09_off)) {
		err = PTR_ERR(ufs_mtk_pins_va09_off);
		ufs_mtk_pins_va09_off = NULL;
		dev_err(hba->dev, "error: ufs_mtk_pins_va09_off init fail\n");
		return err;
	}

	pinctrl_select_state(ufs_mtk_pinctrl, ufs_mtk_pins_va09_on);

	return err;
}

int ufs_mtk_pltfrm_res_req(struct ufs_hba *hba, u32 option)
{
	if (option == UFS_MTK_RESREQ_DMA_OP) {

		/* request resource for DMA operations, e.g., DRAM */

		ufshcd_vops_deepidle_resource_req(hba,
		  SPM_RESOURCE_MAINPLL | SPM_RESOURCE_DRAM | SPM_RESOURCE_CK_26M);

	} else if (option == UFS_MTK_RESREQ_MPHY_NON_H8) {

		/* request resource for mphy not in H8, e.g., main PLL, 26 mhz clock */

		ufshcd_vops_deepidle_resource_req(hba,
		  SPM_RESOURCE_MAINPLL | SPM_RESOURCE_CK_26M);
	}

	return 0;
}

int ufs_mtk_pltfrm_resume(struct ufs_hba *hba)
{
	int ret = 0;
	u32 reg;

	/* Enable MPHY 26MHz ref clock */
	clk_buf_ctrl(CLK_BUF_UFS, true);

	/* Set GPIO to turn on VA09 LDO */
	if (ufs_mtk_pins_va09_on)
		pinctrl_select_state(ufs_mtk_pinctrl, ufs_mtk_pins_va09_on);

	/* wait 156 us to stablize VA09 */
	udelay(156);

	/* Step 1: Set RG_VA09_ON to 1 */
	mt_secure_call(MTK_SIP_KERNEL_UFS_CTL, (1 << 0), 1, 0);

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
	mt_secure_call(MTK_SIP_KERNEL_UFS_CTL, (1 << 0), 0, 0);

	/* delay awhile to satisfy T_HIBERNATE */
	mdelay(15);

	/* Disable MPHY 26MHz ref clock in H8 mode */
	clk_buf_ctrl(CLK_BUF_UFS, false);

	/* Set GPIO to turn off VA09 LDO */
	if (ufs_mtk_pins_va09_off)
		pinctrl_select_state(ufs_mtk_pinctrl, ufs_mtk_pins_va09_off);

#if 0
	/* TEST ONLY: emulate UFSHCI power off by HCI SW reset */
	ufs_mtk_pltfrm_host_sw_rst(hba, SW_RST_TARGET_UFSHCI);
#endif

	return ret;
}


