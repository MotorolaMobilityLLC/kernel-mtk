#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/mt_spm_sleep.h>

#include "ccci_core.h"
#include "ccci_platform.h"
#include "ccif_c2k_platform.h"
#include "modem_ccif.h"
#include "modem_reg_base.h"

#include <mach/upmu_common.h>
#include <mach/mt_boot.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#define TAG "cif"

#if !defined(CONFIG_MTK_LEGACY)
#include <linux/clk.h>
static struct clk *clk_scp_sys_md2_main;
static struct clk *clk_scp_sys_md3_main;
#else
#include <mach/mt_clkmgr.h>
#endif

#define PCCIF_BUSY (0x4)
#define PCCIF_TCHNUM (0xC)
#define PCCIF_ACK (0x14)
#define PCCIF_CHDATA (0x100)
#define PCCIF_SRAM_SIZE (512)

int md_ccif_get_modem_hw_info(struct platform_device *dev_ptr,
			      struct ccci_dev_cfg *dev_cfg,
			      struct md_hw_info *hw_info)
{
	struct device_node *node = NULL;

	memset(dev_cfg, 0, sizeof(struct ccci_dev_cfg));
	memset(hw_info, 0, sizeof(struct md_hw_info));

#ifdef CONFIG_OF
	if (dev_ptr->dev.of_node == NULL) {
		CCCI_ERR_MSG(dev_cfg->index, TAG, "modem OF node NULL\n");
		return -1;
	}

	of_property_read_u32(dev_ptr->dev.of_node, "cell-index",
			     &dev_cfg->index);
	CCCI_INF_MSG(dev_cfg->index, TAG, "modem hw info get idx:%d\n",
		     dev_cfg->index);
	if (!get_modem_is_enabled(dev_cfg->index)) {
		CCCI_ERR_MSG(dev_cfg->index, TAG, "modem %d not enable, exit\n",
			     dev_cfg->index + 1);
		return -1;
	}
#else
	struct ccci_dev_cfg *dev_cfg_ptr =
	    (struct ccci_dev_cfg *)dev->dev.platform_data;
	dev_cfg->index = dev_cfg_ptr->index;

	CCCI_INF_MSG(dev_cfg->index, TAG, "modem hw info get idx:%d\n",
		     dev_cfg->index);
	if (!get_modem_is_enabled(dev_cfg->index)) {
		CCCI_ERR_MSG(dev_cfg->index, TAG, "modem %d not enable, exit\n",
			     dev_cfg->index + 1);
		return -1;
	}
#endif

	switch (dev_cfg->index) {
	case 1:		/*MD_SYS2 */
#ifdef CONFIG_OF
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,major",
				     &dev_cfg->major);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,minor_base",
				     &dev_cfg->minor_base);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,capability",
				     &dev_cfg->capability);

		hw_info->ap_ccif_base = of_iomap(dev_ptr->dev.of_node, 0);
		/*hw_info->md_ccif_base = hw_info->ap_ccif_base+0x1000; */
		node = of_find_compatible_node(NULL, NULL, "mediatek,MD_CCIF1");
		hw_info->md_ccif_base = of_iomap(node, 0);

		hw_info->ap_ccif_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->md_wdt_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 1);

		/*Device tree using none flag to register irq, sensitivity has set at "irq_of_parse_and_map" */
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;
#endif

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD2_RGU_BASE;
		hw_info->md_boot_slave_Vector = MD2_BOOT_VECTOR;
		hw_info->md_boot_slave_Key = MD2_BOOT_VECTOR_KEY;
		hw_info->md_boot_slave_En = MD2_BOOT_VECTOR_EN;

#if !defined(CONFIG_MTK_LEGACY)
		clk_scp_sys_md2_main =
		    devm_clk_get(&dev_ptr->dev, "scp-sys-md2-main");
		if (IS_ERR(clk_scp_sys_md2_main)) {
			CCCI_ERR_MSG(dev_cfg->index, TAG,
				     "modem %d get scp-sys-md2-main failed\n",
				     dev_cfg->index + 1);
			return -1;
		}
#endif
		break;
	case 2:		/*MD_SYS3 */
#ifdef CONFIG_OF
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,major",
				     &dev_cfg->major);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,minor_base",
				     &dev_cfg->minor_base);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,capability",
				     &dev_cfg->capability);

		hw_info->ap_ccif_base = of_iomap(dev_ptr->dev.of_node, 0);
		/*hw_info->md_ccif_base = hw_info->ap_ccif_base+0x1000; */
		node = of_find_compatible_node(NULL, NULL, "mediatek,MD_CCIF1");
		hw_info->md_ccif_base = of_iomap(node, 0);

		hw_info->ap_ccif_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->md_wdt_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 1);

		/*Device tree using none flag to register irq, sensitivity has set at "irq_of_parse_and_map" */
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;

		hw_info->md1_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 1);
		hw_info->md3_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 2);

		node =
		    of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
		hw_info->infra_ao_base = (unsigned long)of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,SLEEP");
		hw_info->sleep_base = (unsigned long)of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,TOPRGU");
		hw_info->toprgu_base = (unsigned long)of_iomap(node, 0);

		CCCI_INF_MSG(dev_cfg->index, TAG,
			     "infra_ao_base=0x%lx, sleep_base=0x%lx, toprgu_base=0x%lx\n",
			     hw_info->infra_ao_base, hw_info->sleep_base,
			     hw_info->toprgu_base);
#endif

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD3_RGU_BASE;

#if !defined(CONFIG_MTK_LEGACY)
		clk_scp_sys_md3_main =
		    devm_clk_get(&dev_ptr->dev, "scp-sys-md2-main");
		if (IS_ERR(clk_scp_sys_md3_main)) {
			CCCI_ERR_MSG(dev_cfg->index, TAG,
				     "modem %d get scp-sys-md2-main failed\n",
				     dev_cfg->index + 1);
			return -1;
		}
#endif

		/*no boot slave for md3 */
		/*
		   hw_info->md_boot_slave_Vector = MD3_BOOT_VECTOR;
		   hw_info->md_boot_slave_Key = MD3_BOOT_VECTOR_KEY;
		   hw_info->md_boot_slave_En = MD3_BOOT_VECTOR_EN;
		 */
		break;
	default:
		return -1;
	}

	CCCI_INF_MSG(dev_cfg->index, TAG,
		     "modem ccif of node get dev_major:%d\n", dev_cfg->major);
	CCCI_INF_MSG(dev_cfg->index, TAG,
		     "modem ccif of node get minor_base:%d\n",
		     dev_cfg->minor_base);
	CCCI_INF_MSG(dev_cfg->index, TAG,
		     "modem ccif of node get capability:%d\n",
		     dev_cfg->capability);

	CCCI_INF_MSG(dev_cfg->index, TAG, "ap_ccif_base:0x%p\n",
		     (void *)hw_info->ap_ccif_base);
	CCCI_INF_MSG(dev_cfg->index, TAG, "ccif_irq_id:%d\n",
		     hw_info->ap_ccif_irq_id);
	CCCI_INF_MSG(dev_cfg->index, TAG, "md_wdt_irq_id:%d\n",
		     hw_info->md_wdt_irq_id);

	return 0;
}

int md_ccif_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	switch (md->index) {
	case MD_SYS2:
		md_ctrl->md_boot_slave_Vector =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Vector,
				    0x4);
		md_ctrl->md_boot_slave_Key =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Key, 0x4);
		md_ctrl->md_boot_slave_En =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_En, 0x4);
		md_ctrl->md_rgu_base =
		    ioremap_nocache(md_ctrl->hw_info->md_rgu_base, 0x40);
		break;
	case MD_SYS3:
		break;
	}
	return 0;
}

/*need modify according to dummy ap*/
int md_ccif_let_md_go(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	if (MD_IN_DEBUG(md)) {
		CCCI_INF_MSG(md->index, TAG, "DBG_FLAG_JTAG is set\n");
		return -1;
	}
	CCCI_INF_MSG(md->index, TAG, "md_ccif_let_md_go\n");
	switch (md->index) {
	case MD_SYS2:
		/*set the start address to let modem to run */
			/*make boot vector programmable */
		ccif_write32(md_ctrl->md_boot_slave_Key, 0, MD2_BOOT_VECTOR_KEY_VALUE);
			/*after remap, MD ROM address is 0 from MD's view */
		ccif_write32(md_ctrl->md_boot_slave_Vector, 0, MD2_BOOT_VECTOR_VALUE);
			/*make boot vector take effect */
		ccif_write32(md_ctrl->md_boot_slave_En, 0, MD2_BOOT_VECTOR_EN_VALUE);
		break;
	case MD_SYS3:
		/*check if meta mode */
		if (is_meta_mode() || get_boot_mode() == FACTORY_BOOT) {
			ccif_write32(md_ctrl->hw_info->infra_ao_base,
				     INFRA_AO_C2K_CONFIG,
				     (ccif_read32
				      (md_ctrl->hw_info->infra_ao_base,
				       INFRA_AO_C2K_CONFIG) | ETS_SEL_BIT));
		}
		/*step 1: set C2K boot mode */
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_CONFIG,
			     (ccif_read32
			      (md_ctrl->hw_info->infra_ao_base,
			       INFRA_AO_C2K_CONFIG) & (~(0x7 << 8))) | (0x5 <<
									8));
		CCCI_INF_MSG(md->index, TAG, "C2K_CONFIG = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_CONFIG));
		/*step 2: config srcclkena selection mask */
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) & (~(0x3 <<
								     4)));
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) | (0x2 << 4));
		CCCI_INF_MSG(md->index, TAG, "C2K_SPM_CTRL = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL));
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) | 0xc);
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) & (~(0x1 << 14)));
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) | (0x1 << 12));
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) | (0x1 << 27));
		CCCI_INF_MSG(md->index, TAG, "SLEEP_CLK_CON = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON));

		/*step 3: PMIC VTCXO_1 enable */
		pmic_config_interface(0x0A02, 0xA12E, 0xFFFF, 0x0);
		/*step 4: reset C2K */
#if 0
		ccif_write32(md_ctrl->hw_info->toprgu_base,
			     TOP_RGU_WDT_SWSYSRST,
			     (ccif_read32
			      (md_ctrl->hw_info->toprgu_base,
			       TOP_RGU_WDT_SWSYSRST) | 0x88000000) & (~(0x1 <<
									15)));
#else
		mtk_wdt_set_c2k_sysrst(1);
#endif
		CCCI_INF_MSG(md->index, TAG,
			     "[C2K] TOP_RGU_WDT_SWSYSRST = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->toprgu_base,
					 TOP_RGU_WDT_SWSYSRST));

		/*step 5: mpu already set */
		/*step 6: wake up C2K */
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) | 0x1);
		while (!
		       ((ccif_read32
			 (md_ctrl->hw_info->infra_ao_base,
			  INFRA_AO_C2K_STATUS) >> 1) & 0x1)) {
			CCCI_INF_MSG(md->index, TAG,
				     "[C2K] C2K_STATUS = 0x%x\n",
				     ccif_read32(md_ctrl->hw_info->
						 infra_ao_base,
						 INFRA_AO_C2K_STATUS));
		}
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) & (~0x1));
		CCCI_INF_MSG(md->index, TAG,
			     "[C2K] C2K_SPM_CTRL = 0x%x, C2K_STATUS = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL),
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_STATUS));
		break;
	}
	return 0;
}

int md_ccif_power_on(struct ccci_modem *md)
{
	int ret = 0;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	switch (md->index) {
	case MD_SYS2:
#if defined(CONFIG_MTK_LEGACY)
		CCCI_INF_MSG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD2);
		CCCI_INF_MSG(md->index, TAG, "Call end md_power_on() ret=%d\n",
			     ret);
#else
		CCCI_INF_MSG(md->index, TAG,
			     "Call start clk_prepare_enable()\n");
		clk_prepare_enable(clk_scp_sys_md2_main);
		CCCI_INF_MSG(md->index, TAG, "Call end clk_prepare_enable()\n");
#endif
		break;
	case MD_SYS3:
#if defined(CONFIG_MTK_LEGACY)
		CCCI_INF_MSG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD2);
		CCCI_INF_MSG(md->index, TAG, "Call end md_power_on() ret=%d\n",
			     ret);
#else
		CCCI_INF_MSG(md->index, TAG,
			     "Call start clk_prepare_enable()\n");
		clk_prepare_enable(clk_scp_sys_md3_main);
		CCCI_INF_MSG(md->index, TAG, "Call end clk_prepare_enable()\n");
#endif

		break;
	}
	CCCI_INF_MSG(md->index, TAG, "md_ccif_power_on:ret=%d\n", ret);
	if (ret == 0 && md->index != MD_SYS3) {
		/*disable MD WDT */
		ccif_write32(md_ctrl->md_rgu_base, WDT_MD_MODE,
			     WDT_MD_MODE_KEY);
	}
	return ret;
}

int md_ccif_power_off(struct ccci_modem *md, unsigned int timeout)
{
	int ret = 0;

	switch (md->index) {
	case MD_SYS2:
#if defined(CONFIG_MTK_LEGACY)
		ret = md_power_off(SYS_MD2, timeout);
#else
		clk_disable_unprepare(clk_scp_sys_md2_main);
#endif
		break;
	case MD_SYS3:
#if defined(CONFIG_MTK_LEGACY)
		ret = md_power_off(SYS_MD1, timeout);
#else
		clk_disable_unprepare(clk_scp_sys_md3_main);
#endif
		break;
	}
	CCCI_INF_MSG(md->index, TAG, "md_ccif_power_off:ret=%d\n", ret);
	return ret;
}

void reset_md1_md3_pccif(struct ccci_modem *md)
{
	unsigned int tx_channel = 0;
	int i;

	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	struct md_hw_info *hw_info = md_ctrl->hw_info;
	/* clear occupied channel */
	while (tx_channel < 16) {
		if (ccif_read32(hw_info->md1_pccif_base, PCCIF_BUSY) & (1<<tx_channel))
			ccif_write32(hw_info->md1_pccif_base, PCCIF_TCHNUM, tx_channel);

		if (ccif_read32(hw_info->md3_pccif_base, PCCIF_BUSY) & (1<<tx_channel))
			ccif_write32(hw_info->md3_pccif_base, PCCIF_TCHNUM, tx_channel);

		tx_channel++;
	}
	/* clear un-ached channel *.
	ccif_write32(hw_info->md1_pccif_base, PCCIF_ACK, ccif_read32(hw_info->md3_pccif_base, PCCIF_BUSY));
	ccif_write32(hw_info->md3_pccif_base, PCCIF_ACK, ccif_read32(hw_info->md1_pccif_base, PCCIF_BUSY));
	/* clear SRAM */
	for (i = 0; i < PCCIF_SRAM_SIZE/sizeof(unsigned int); i++) {
		ccif_write32(hw_info->md1_pccif_base, PCCIF_CHDATA + i*sizeof(unsigned int), 0);
		ccif_write32(hw_info->md3_pccif_base, PCCIF_CHDATA + i*sizeof(unsigned int), 0);
	}

	pr_debug("[C2K] Dump MD1 PCCIF\n");
	ccci_mem_dump(-1, hw_info->md1_pccif_base, 0x300);
	pr_debug("[C2K] Dump MD3 PCCIF\n");
	ccci_mem_dump(-1, hw_info->md3_pccif_base, 0x300);
}

