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

#include "mtk_idle.h"
#include "mtk_spm_resource_req.h"

/* #define UFS_DEV_REF_CLK_CONTROL */

static void __iomem *ufs_mtk_mmio_base_infracfg_ao;
static void __iomem *ufs_mtk_mmio_base_pericfg;
static void __iomem *ufs_mtk_mmio_base_ufs_mphy;

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
	#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(UFS_DEV_REF_CLK_CONTROL)
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

#if !defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(UFS_DEV_REF_CLK_CONTROL)
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
	struct device_node *node_pericfg;
	int err = 0;

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
	return 0;
}

int ufs_mtk_pltfrm_suspend(struct ufs_hba *hba)
{
	return 0;
}

