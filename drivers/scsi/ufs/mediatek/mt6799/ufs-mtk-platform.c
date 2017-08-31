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

#include "ufs.h"
#include "ufshcd.h"
#include "unipro.h"
#include "ufs-mtk.h"
#include "ufs-mtk-platform.h"

#include "mtk_spm_resource_req.h"
#include <mtk_clkbuf_ctl.h>
#include "mtk_idle.h"
#ifdef MTK_UFS_HQA
#include <mtk_reboot.h>
#include <upmu_common.h>
#endif

static void __iomem *ufs_mtk_mmio_base_gpio;
static void __iomem *ufs_mtk_mmio_base_infracfg_ao;
static void __iomem *ufs_mtk_mmio_base_pericfg;
static void __iomem *ufs_mtk_mmio_base_ufs_mphy;

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

	dev_err(hba->dev, "%s: mvg_spoh reset delay time 0x%x\n", __func__, time);
	udelay(time);
}
void wdt_pmic_full_reset(void)
{
	/* pmic full reset settng */
	pmic_set_register_value(PMIC_RG_CRST, 1);
}
#endif

#include <mt-plat/upmu_common.h>
#define PMIC_REG_MASK (0xFFFF)
#define PMIC_REG_SHIFT (0)
void ufs_mtk_pltfrm_gpio_trigger_and_debugInfo_dump(struct ufs_hba *hba)
{
	#ifdef CONFIG_MTK_UFS_DEGUG_GPIO_TRIGGER
	u32 reg;
	#endif
	u32 pmic_cw11 = 0, pmic_cw02 = 0, pmic_cw00 = 0;
	int vcc_enabled, vcc_value, vccq2_enabled;

	#ifdef CONFIG_MTK_UFS_DEGUG_GPIO_TRIGGER
	/* mt_set_gpio_out() will have latency, set GPIO register to high directly */
	/* Set DOUT to high */
	reg = readl(ufs_mtk_mmio_base_gpio + 0x150);
	reg = reg | (0x1 << 17);
	writel(reg, ufs_mtk_mmio_base_gpio + 0x150);
	#endif

	/* Get XO_UFS ufs 26MHz ref. clock info. */
	pmic_read_interface(MT6335_DCXO_CW11, &pmic_cw11, PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW02, &pmic_cw02, PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK, PMIC_REG_SHIFT);

	/* Get vcc(3V) and vccq2(1.8V) info.
	  * Note:
	  * 1. regulator_is_enabled/regulator_get_voltage is not able to use in IRQ context because it acquire mutex.
	  * 2. mt6799 does not have raw pmic API to read vccq2 voltage because it is not able to change on this plat.
	  */
	vcc_enabled = pmic_get_register_value(PMIC_DA_QI_VEMC_EN);
	vcc_value = pmic_get_register_value(PMIC_RG_VEMC_VOSEL);
	vccq2_enabled = pmic_get_register_value(PMIC_DA_QI_VUFS18_EN);

	/* Print all after information are retrieved */
	dev_err(hba->dev, "pmic_cw11 0x%x, pmic_cw02 0x%x, pmic_cw00 0x%x!!!\n",
			pmic_cw11, pmic_cw02, pmic_cw00);
	dev_err(hba->dev, "vcc_enabled:%d, vcc_value:%d, vccq2_enabled:%d!!!\n",
			vcc_enabled, vcc_value, vccq2_enabled);
}

#ifdef CONFIG_MTK_UFS_DEGUG_GPIO_TRIGGER
#include <mt-plat/mtk_gpio.h>
#define gpioPin (177UL)
void ufs_mtk_pltfrm_gpio_trigger_init(struct ufs_hba *hba)
{
	/* Set gpio mode */
	mt_set_gpio_mode(gpioPin, 0x0UL);
	/* Set gpio output */
	mt_set_gpio_dir(gpioPin, 0x1UL);
	/* Set gpio default low */
	mt_set_gpio_out(gpioPin, 0x0UL);
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
	reg = reg | (1 << REG_UFS_PERICFG_LDO_N_BIT);
	writel(reg, ufs_mtk_mmio_base_pericfg + REG_UFS_PERICFG);

	udelay(10);

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
 * @return: UFS_H8(0) needs to disable 26MHz ref clk.
 *              UFS_H8_SUSPEND(1) is after ufshcd_suspend() 26MHz ref clk already disabled by suspend callback.
 *                                              SODI/DeepIlde don't need disable it.
 *              Negative error code otherwise.
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
	struct device_node *node_infracfg_ao;
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


	/* get ufs_mtk_mmio_base_infracfg_ao */

	node_infracfg_ao = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-infracfg_ao");

	if (node_infracfg_ao) {
		ufs_mtk_mmio_base_infracfg_ao = of_iomap(node_infracfg_ao, 0);

		if (IS_ERR(*(void **)&ufs_mtk_mmio_base_infracfg_ao)) {
			err = PTR_ERR(*(void **)&ufs_mtk_mmio_base_infracfg_ao);
			dev_err(hba->dev, "error: ufs_mtk_mmio_base_infracfg_ao init fail\n");
			ufs_mtk_mmio_base_infracfg_ao = NULL;
		}
	} else
		dev_err(hba->dev, "error: node_infracfg_ao init fail\n");

	/* get ufs_mtk_mmio_base_pericfg */

	node_pericfg = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-pericfg");

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

	/* VA09 LDO will be power on by SPM (Step1: SPM turn on VA09 LDO Step2: SPM set RG_VA09_ON to 1'b1	)
	 * Before VA09 LDO is enabled, some sw flow of mphy needs to be  done here.
	*/
	/* step6: release DA_MP_PLL_PWR_ON=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg | (0x1 << 11);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg & ~(0x1 << 10);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);

	/* step5: release DA_MP_PLL_ISO_EN=1 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg & ~(0x1 << 9);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg & ~(0x1 << 8);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);

	/* step4: release DA_MP_CDR_PWR_ON=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg | (0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg & ~(0x1 << 17);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);

	/* step3: release DA_MP_CDR_ISO_EN=1 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg & ~(0x1 << 20);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg & ~(0x1 << 19);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);

	/* step2: release DA_MP_RX0_SQ_EN=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = reg | (0x1 << 1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = reg & ~(0x1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);

	/* Delay 1us to wait DIFZ stable */
	udelay(1);

	/* step1: release DIFZ hihg */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa09c);
	reg = reg & ~(0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa09c);

	return ret;
}

int ufs_mtk_pltfrm_suspend(struct ufs_hba *hba)
{
	int ret = 0;
	u32 reg;

	/*
	 * VA09 LDO will be shutdwon by SPM (Step1: SPM set RG_VA09_ON to 1'b0 Step2: SPM turn off VA09 LDO )
	 * Before VA09 LDO shutdown, some sw flow of mphy needs to be  done here.
	 */

	/* step1: force DIFZ hihg */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa09c);
	reg = reg | (0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa09c);

	/* step2: force DA_MP_RX0_SQ_EN=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = reg | (0x1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);
	reg = reg & ~(0x1 << 1);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xa0ac);

	/* step3: force DA_MP_CDR_ISO_EN=1 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg | (0x1 << 19);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg | (0x1 << 20);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);

	/* step4: force DA_MP_CDR_PWR_ON=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg | (0x1 << 17);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0xb044);
	reg = reg & ~(0x1 << 18);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0xb044);

	/* step5: force DA_MP_PLL_ISO_EN=1 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg | (0x1 << 8);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg | (0x1 << 9);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);

	/* step6: force DA_MP_PLL_PWR_ON=0 */
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg | (0x1 << 10);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = readl(ufs_mtk_mmio_base_ufs_mphy + 0x008c);
	reg = reg & ~(0x1 << 11);
	writel(reg, ufs_mtk_mmio_base_ufs_mphy + 0x008c);

	/* delay awhile to satisfy T_HIBERNATE */
	mdelay(15);

	/* Disable MPHY 26MHz ref clock in H8 mode */
	clk_buf_ctrl(CLK_BUF_UFS, false);

#if 0
	/* TEST ONLY: emulate UFSHCI power off by HCI SW reset */
	ufs_mtk_pltfrm_host_sw_rst(hba, SW_RST_TARGET_UFSHCI);
#endif

	return ret;
}

