/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG "[WMT-CONSYS-HW]"

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#if defined(CONFIG_MTK_CLKMGR)
#include <mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif /* defined(CONFIG_MTK_CLKMGR) */
#include <linux/delay.h>
#include <linux/memblock.h>
#include "osal_typedef.h"
#include "mt6797.h"
#include "mtk_wcn_consys_hw.h"
#include <linux/platform_device.h>

#if CONSYS_EMI_MPU_SETTING
#include <emi_mpu.h>
#endif

#if CONSYS_PMIC_CTRL_ENABLE
#include <upmu_common.h>
#include <linux/regulator/consumer.h>
#endif

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

#include <linux/of_reserved_mem.h>

#if CONSYS_CLOCK_BUF_CTRL
#include <mt_clkbuf_ctl.h>
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable);
static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable);
static VOID consys_hw_spm_clk_gating_enable(VOID);
static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable);
static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable);
static INT32 polling_consys_chipid(VOID);
static VOID consys_acr_reg_setting(VOID);
static VOID consys_afe_reg_setting(VOID);
static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable);
static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable);
static INT32 consys_hw_vcn28_ctrl(UINT32 enable);
static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable);
static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable);
static UINT32 consys_soc_chipid_get(VOID);
static INT32 consys_emi_mpu_set_region_protection(VOID);
static UINT32 consys_emi_set_remapping_reg(VOID);
static INT32 bt_wifi_share_v33_spin_lock_init(VOID);
static INT32 consys_clk_get_from_dts(struct platform_device *pdev);
static INT32 consys_pmic_get_from_dts(struct platform_device *pdev);
static INT32 consys_read_irq_info_from_dts(INT32 *irq_num, UINT32 *irq_flag);
static INT32 consys_read_reg_from_dts(VOID);
static UINT32 consys_read_cpupcr(VOID);
static VOID force_trigger_assert_debug_pin(VOID);
static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if CONSYS_BT_WIFI_SHARE_V33
BT_WIFI_V33_STATUS gBtWifiV33;
#endif

/* CCF part */
#if !defined(CONFIG_MTK_CLKMGR)
struct clk *clk_scp_conn_main;	/*ctrl conn_power_on/off */
#if CONSYS_AHB_CLK_MAGEMENT
struct clk *clk_infra_conn_main;	/*ctrl infra_connmcu_bus clk */
#endif
#endif /* !defined(CONFIG_MTK_CLKMGR) */


/* PMIC part */
#if CONSYS_PMIC_CTRL_ENABLE
#if !defined(CONFIG_MTK_PMIC_LEGACY)
struct regulator *reg_VCN18;
struct regulator *reg_VCN28;
struct regulator *reg_VCN33_BT;
struct regulator *reg_VCN33_WIFI;
#endif
#endif

EMI_CTRL_STATE_OFFSET mtk_wcn_emi_state_off = {
	.emi_apmem_ctrl_state = EXP_APMEM_CTRL_STATE,
	.emi_apmem_ctrl_host_sync_state = EXP_APMEM_CTRL_HOST_SYNC_STATE,
	.emi_apmem_ctrl_host_sync_num = EXP_APMEM_CTRL_HOST_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_state = EXP_APMEM_CTRL_CHIP_SYNC_STATE,
	.emi_apmem_ctrl_chip_sync_num = EXP_APMEM_CTRL_CHIP_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_addr = EXP_APMEM_CTRL_CHIP_SYNC_ADDR,
	.emi_apmem_ctrl_chip_sync_len = EXP_APMEM_CTRL_CHIP_SYNC_LEN,
	.emi_apmem_ctrl_chip_print_buff_start = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START,
	.emi_apmem_ctrl_chip_print_buff_len = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN,
	.emi_apmem_ctrl_chip_print_buff_idx = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX,
	.emi_apmem_ctrl_chip_int_status = EXP_APMEM_CTRL_CHIP_INT_STATUS,
	.emi_apmem_ctrl_chip_paded_dump_end = EXP_APMEM_CTRL_CHIP_PAGED_DUMP_END,
	.emi_apmem_ctrl_host_outband_assert_w1 = EXP_APMEM_CTRL_HOST_OUTBAND_ASSERT_W1,
	.emi_apmem_ctrl_chip_page_dump_num = EXP_APMEM_CTRL_CHIP_PAGE_DUMP_NUM,
};

CONSYS_EMI_ADDR_INFO mtk_wcn_emi_addr_info = {
	.emi_phy_addr = CONSYS_EMI_FW_PHY_BASE,
	.paged_trace_off = CONSYS_EMI_PAGED_TRACE_OFFSET,
	.paged_dump_off = CONSYS_EMI_PAGED_DUMP_OFFSET,
	.full_dump_off = CONSYS_EMI_FULL_DUMP_OFFSET,
	.p_ecso = &mtk_wcn_emi_state_off,
};

WMT_CONSYS_IC_OPS consys_ic_ops = {
	.consys_ic_clock_buffer_ctrl = consys_clock_buffer_ctrl,
	.consys_ic_hw_reset_bit_set = consys_hw_reset_bit_set,
	.consys_ic_hw_spm_clk_gating_enable = consys_hw_spm_clk_gating_enable,
	.consys_ic_hw_power_ctrl = consys_hw_power_ctrl,
	.consys_ic_ahb_clock_ctrl = consys_ahb_clock_ctrl,
	.polling_consys_ic_chipid = polling_consys_chipid,
	.consys_ic_acr_reg_setting = consys_acr_reg_setting,
	.consys_ic_afe_reg_setting = consys_afe_reg_setting,
	.consys_ic_hw_vcn18_ctrl = consys_hw_vcn18_ctrl,
	.consys_ic_vcn28_hw_mode_ctrl = consys_vcn28_hw_mode_ctrl,
	.consys_ic_hw_vcn28_ctrl = consys_hw_vcn28_ctrl,
	.consys_ic_hw_wifi_vcn33_ctrl = consys_hw_wifi_vcn33_ctrl,
	.consys_ic_hw_bt_vcn33_ctrl = consys_hw_bt_vcn33_ctrl,
	.consys_ic_soc_chipid_get = consys_soc_chipid_get,
	.consys_ic_emi_mpu_set_region_protection = consys_emi_mpu_set_region_protection,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg,
	.ic_bt_wifi_share_v33_spin_lock_init = bt_wifi_share_v33_spin_lock_init,
	.consys_ic_clk_get_from_dts = consys_clk_get_from_dts,
	.consys_ic_pmic_get_from_dts = consys_pmic_get_from_dts,
	.consys_ic_read_irq_info_from_dts = consys_read_irq_info_from_dts,
	.consys_ic_read_reg_from_dts = consys_read_reg_from_dts,
	.consys_ic_read_cpupcr = consys_read_cpupcr,
	.ic_force_trigger_assert_debug_pin = force_trigger_assert_debug_pin,
	.consys_ic_soc_get_emi_phy_add = consys_soc_get_emi_phy_add,
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
UINT32 gJtagCtrl = 0;
#if CONSYS_ENALBE_SET_JTAG
#define JTAG_ADDR1_BASE 0x10005000
#define JTAG_ADDR2_BASE 0x10002000
PINT8 jtag_addr1 = (PINT8)JTAG_ADDR1_BASE;
PINT8 jtag_addr2 = (PINT8)JTAG_ADDR2_BASE;

#define JTAG1_REG_WRITE(addr, value) \
	writel(value, ((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))
#define JTAG1_REG_READ(addr) \
	readl(((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))

#define JTAG2_REG_WRITE(addr, value) \
	writel(value, ((PUINT32)(jtag_addr2+(addr-JTAG_ADDR2_BASE))))
#define JTAG2_REG_READ(addr) \
	readl(((PUINT32)(jtag_addr2+(addr-JTAG_ADDR2_BASE))))
#endif

INT32 mtk_wcn_consys_jtag_set_for_mcu(VOID)
{
#if CONSYS_ENALBE_SET_JTAG
	INT32 iRet = -1;
	UINT32 tmp = 0;

	if (gJtagCtrl) {
		WMT_PLAT_DBG_FUNC("WCN jtag_set_for_mcu start...\n");
		jtag_addr1 = ioremap(JTAG_ADDR1_BASE, 0x1000);
		if (jtag_addr1 == 0) {
			WMT_PLAT_ERR_FUNC("remap jtag_addr1 fail!\n");
			return iRet;
		}
		WMT_PLAT_DBG_FUNC("jtag_addr1 = 0x%p\n", jtag_addr1);
		jtag_addr2 = ioremap(JTAG_ADDR2_BASE, 0x1000);
		if (jtag_addr2 == 0) {
			WMT_PLAT_ERR_FUNC("remap jtag_addr2 fail!\n");
			return iRet;
		}
		WMT_PLAT_DBG_FUNC("jtag_addr2 = 0x%p\n", jtag_addr2);

		tmp = JTAG1_REG_READ(0x10005330);
		tmp &= 0xff00ffff;
		tmp |= 0x00550000;
		JTAG1_REG_WRITE(0x10005330, tmp);
		WMT_PLAT_DBG_FUNC("JTAG set reg:%x, val:%x\n", 0x10005330, tmp);

		tmp = JTAG1_REG_READ(0x10005340);
		tmp &= 0xfff00000;
		tmp |= 0x00055555;
		JTAG1_REG_WRITE(0x10005340, tmp);
		WMT_PLAT_DBG_FUNC("JTAG set reg:%x, val:%x\n", 0x10005340, tmp);

		tmp = JTAG2_REG_READ(0x100020F0);
		tmp &= 0xfffffc0c;
		tmp |= 0x000003f3;
		JTAG2_REG_WRITE(0x100020F0, tmp);
		WMT_PLAT_DBG_FUNC("JTAG set reg:%x, val:%x\n", 0x100020F0, tmp);

		tmp = JTAG2_REG_READ(0x100020b0);
		tmp &= 0xfffffe0c;
		tmp |= 0x00000180;
		JTAG2_REG_WRITE(0x100020b0, tmp);
		WMT_PLAT_DBG_FUNC("JTAG set reg:%x, val:%x\n", 0x100020b0, tmp);

		tmp = JTAG2_REG_READ(0x100020d0);
		tmp &= 0xfffffe0c;
		JTAG2_REG_WRITE(0x100020d0, tmp);
		WMT_PLAT_DBG_FUNC("JTAG set reg:%x, val:%x\n", 0x100020d0, tmp);
	}
#endif
	return 0;
}

UINT32 mtk_wcn_consys_jtag_flag_ctrl(UINT32 en)
{
	WMT_PLAT_INFO_FUNC("%s jtag set for MCU\n", en ? "enable" : "disable");
	gJtagCtrl = en;
	return 0;
}

static INT32 consys_clk_get_from_dts(struct platform_device *pdev)
{
#ifdef CONFIG_OF		/*use DT */
#if !defined(CONFIG_MTK_CLKMGR)
	clk_scp_conn_main = devm_clk_get(&pdev->dev, "conn");
	if (IS_ERR(clk_scp_conn_main)) {
		WMT_PLAT_ERR_FUNC("[CCF]cannot get clk_scp_conn_main clock.\n");
		return PTR_ERR(clk_scp_conn_main);
	}
	WMT_PLAT_DBG_FUNC("[CCF]clk_scp_conn_main=%p\n", clk_scp_conn_main);

#if CONSYS_AHB_CLK_MAGEMENT
	clk_infra_conn_main = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(clk_infra_conn_main)) {
		WMT_PLAT_ERR_FUNC("[CCF]cannot get clk_infra_conn_main clock.\n");
		return PTR_ERR(clk_infra_conn_main);
	}
	WMT_PLAT_DBG_FUNC("[CCF]clk_infra_conn_main=%p\n", clk_infra_conn_main);
#endif
#endif /* !defined(CONFIG_MTK_CLKMGR) */
#endif
	return 0;
}

static INT32 consys_pmic_get_from_dts(struct platform_device *pdev)
{
#ifdef CONFIG_OF		/*use DT */
#if CONSYS_PMIC_CTRL_ENABLE
#if !defined(CONFIG_MTK_PMIC_LEGACY)
	reg_VCN18 = regulator_get(&pdev->dev, "vcn18");
	if (!reg_VCN18)
		WMT_PLAT_ERR_FUNC("Regulator_get VCN_1V8 fail\n");
	reg_VCN28 = regulator_get(&pdev->dev, "vcn28");
	if (!reg_VCN28)
		WMT_PLAT_ERR_FUNC("Regulator_get VCN_2V8 fail\n");
	reg_VCN33_BT = regulator_get(&pdev->dev, "vcn33_bt");
	if (!reg_VCN33_BT)
		WMT_PLAT_ERR_FUNC("Regulator_get VCN33_BT fail\n");
	reg_VCN33_WIFI = regulator_get(&pdev->dev, "vcn33_wifi");
	if (!reg_VCN33_WIFI)
		WMT_PLAT_ERR_FUNC("Regulator_get VCN33_WIFI fail\n");
#endif
#endif
#endif
	return 0;
}

static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable)
{
	return 0;
}

static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable)
{
#ifdef CONFIG_OF		/*use DT */
	if (enable) {
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_CONN2AP_SLEEP_MASK_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_CONN2AP_SLEEP_MASK_OFFSET) |
				 CONSYS_CONN2AP_SLEEP_MASK_BIT);
		/*3.assert CONNSYS CPU SW reset  0x10007018 "[12]=1'b1  [31:24]=8'h88 (key)" */
		/*CONSYS_REG_WRITE((conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET),
				 CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) |
				 CONSYS_CPU_SW_RST_BIT | CONSYS_CPU_SW_RST_CTRL_KEY);*/
		mtk_wdt_swsysret_config((1 << 12), 1);
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				  CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET));
	} else {
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88 (key)" */
		/*CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				 (CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 ~CONSYS_CPU_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);*/
		mtk_wdt_swsysret_config((1 << 12), 0);
	}
#else
	if (enable) {
		/*3.assert CONNSYS CPU SW reset  0x10007018  "[12]=1'b1  [31:24]=8'h88 (key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				 (CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) | CONSYS_CPU_SW_RST_BIT |
				  CONSYS_CPU_SW_RST_CTRL_KEY));
	} else {
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88(key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				 (CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) & ~CONSYS_CPU_SW_RST_BIT) |
				 CONSYS_CPU_SW_RST_CTRL_KEY);
	}
#endif
}

static VOID consys_hw_spm_clk_gating_enable(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	/*turn on SPM clock gating enable PWRON_CONFG_EN  0x10006000  32'h0b160001 */
	CONSYS_REG_WRITE((conn_reg.spm_base + CONSYS_PWRON_CONFG_EN_OFFSET), CONSYS_PWRON_CONFG_EN_VALUE);
#else
	/*turn on SPM clock gating enable PWRON_CONFG_EN 0x10006000 32'h0b160001 */
	CONSYS_REG_WRITE(CONSYS_PWRON_CONFG_EN_REG, CONSYS_PWRON_CONFG_EN_VALUE);
#endif
}

static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	INT32 iRet = -1;
#endif

	if (enable) {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
#if defined(CONFIG_MTK_CLKMGR)
		iRet = conn_power_on();	/* consult clkmgr owner. */
		if (iRet)
			WMT_PLAT_ERR_FUNC("conn_power_on fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("conn_power_on ok\n");
#else
		iRet = clk_prepare_enable(clk_scp_conn_main);
		if (iRet)
			WMT_PLAT_ERR_FUNC("clk_prepare_enable(clk_scp_conn_main) fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("clk_prepare_enable(clk_scp_conn_main) ok\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */

#else

#ifdef CONFIG_OF		/*use DT */
		/*2.write conn_top1_pwr_on=1, power on conn_top1 0x10006280 [2]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_BIT);
		/*3.write conn_top1_pwr_on_s=1, power on conn_top1 0x1000632c [3]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_S_BIT);
		/*4.read conn_top1_pwr_on_ack =1, power on ack ready 0x10006180 [1] */
		while (0 == (CONSYS_PWR_ON_ACK_BIT & CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET)))
			NULL;
		/*5.read conn_top1_pwr_on_ack_s =1, power on ack ready 0x10006184 [1] */
		while (0 == (CONSYS_PWR_CONN_ACK_S_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET)))
			NULL;

		/*6.write conn_clk_dis=0, enable connsys clock 0x10006280 [4]  1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_CLK_CTRL_BIT);
		/*7.wait 1us    */
		udelay(1);

		/*9.release connsys ISO, conn_top1_iso_en=0 0x10006280 [1]  1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_SPM_PWR_ISO_S_BIT);
		/*10.release SW reset of connsys, conn_ap_sw_rst_b=1  0x10006280[0]   1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_RST_BIT);
		/*disable AXI BUS protect */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET) &
				 ~CONSYS_PROT_MASK);
		while (CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_STA1_OFFSET) & CONSYS_PROT_MASK)
			NULL;
#else
		/*2.write conn_top1_pwr_on=1, power on conn_top1 0x10006280 [2]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ON_BIT);
		/*3.read conn_top1_pwr_on_ack =1, power on ack ready 0x1000660C [1] */
		while (0 == (CONSYS_PWR_ON_ACK_BIT & CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_REG)))
			NULL;
		/*5.write conn_top1_pwr_on_s=1, power on conn_top1 0x10006280 [3]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ON_S_BIT);
		/*6.write conn_clk_dis=0, enable connsys clock 0x10006280 [4]  1'b0 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_CLK_CTRL_BIT);
		/*7.wait 1us    */
		udelay(1);
		/*8.read conn_top1_pwr_on_ack_s =1, power on ack ready 0x10006610 [1]  */
		while (0 == (CONSYS_PWR_CONN_ACK_S_BIT & CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_S_REG)))
			NULL;
		/*9.release connsys ISO, conn_top1_iso_en=0 0x10006280 [1]  1'b0 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_SPM_PWR_ISO_S_BIT);
		/*10.release SW reset of connsys, conn_ap_sw_rst_b=1 0x10006280[0] 1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_RST_BIT);
		/*disable AXI BUS protect */
		CONSYS_REG_WRITE(CONSYS_TOPAXI_PROT_EN, CONSYS_REG_READ(CONSYS_TOPAXI_PROT_EN) & ~CONSYS_PROT_MASK);
		while (CONSYS_REG_READ(CONSYS_TOPAXI_PROT_STA1) & CONSYS_PROT_MASK)
			NULL;
#endif /* CONFIG_OF */
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	} else {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
#if defined(CONFIG_MTK_CLKMGR)
		/*power off connsys by API (MT6582, MT6572 are different) API: conn_power_off() */
		iRet = conn_power_off();	/* consult clkmgr owner */
		if (iRet)
			WMT_PLAT_ERR_FUNC("conn_power_off fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("conn_power_off ok\n");
#else
		clk_disable_unprepare(clk_scp_conn_main);
		WMT_PLAT_DBG_FUNC("clk_disable_unprepare(clk_scp_conn_main) calling\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */

#else

#ifdef CONFIG_OF		/*use DT */
		{
			INT32 count = 0;

			CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET,
					 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET) |
					 CONSYS_PROT_MASK);
			while ((CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_STA1_OFFSET) &
				CONSYS_PROT_MASK) != CONSYS_PROT_MASK) {
				count++;
				if (count > 1000)
					break;
			}
		}
		/*assert SW reset of connsys, conn_ap_sw_rst_b=0  0x1000632c[0] 1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_SPM_PWR_RST_BIT);
		/*release connsys ISO, conn_top1_iso_en=1  0x1000632c [1]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ISO_S_BIT);
		/*write conn_clk_dis=1, disable connsys clock  0x10006280 [4]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_CLK_CTRL_BIT);
		/*wait 1us      */
		udelay(1);
		/*write conn_top1_pwr_on=0, power off conn_top1 0x10006280 [3:2] 2'b00 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~(CONSYS_SPM_PWR_ON_BIT | CONSYS_SPM_PWR_ON_S_BIT));
#else
		{
			INT32 count = 0;

			CONSYS_REG_WRITE(CONSYS_TOPAXI_PROT_EN,
					 CONSYS_REG_READ(CONSYS_TOPAXI_PROT_EN) | CONSYS_PROT_MASK);
			while ((CONSYS_REG_READ(CONSYS_TOPAXI_PROT_STA1) & CONSYS_PROT_MASK) != CONSYS_PROT_MASK) {
				count++;
				if (count > 1000)
					break;
			}

		}
		/*release connsys ISO, conn_top1_iso_en=1 0x10006280 [1]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ISO_S_BIT);
		/*assert SW reset of connsys, conn_ap_sw_rst_b=0 0x10006280[0] 1'b0  */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_SPM_PWR_RST_BIT);
		/*write conn_clk_dis=1, disable connsys clock 0x10006280 [4]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_CLK_CTRL_BIT);
		/*wait 1us      */
		udelay(1);
		/*write conn_top1_pwr_on=0, power off conn_top1 0x10006280 [3:2] 2'b00 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG, CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) &
		~(CONSYS_SPM_PWR_ON_BIT | CONSYS_SPM_PWR_ON_S_BIT));
#endif /* CONFIG_OF */
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	}

#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	return iRet;
#else
	return 0;
#endif
}

static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_AHB_CLK_MAGEMENT

	if (enable) {
		/*enable AP bus clock : connmcu_bus_pd  API: enable_clock() ++?? */
#if defined(CONFIG_MTK_CLKMGR)
		enable_clock(MT_CG_INFRA_CONNMCU_BUS, "WCN_MOD");
		WMT_PLAT_DBG_FUNC("enable MT_CG_INFRA_CONNMCU_BUS CLK\n");
#else
		INT32 iRet = -1;

		iRet = clk_prepare_enable(clk_infra_conn_main);
		if (iRet)
			WMT_PLAT_ERR_FUNC("clk_prepare_enable(clk_infra_conn_main) fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("[CCF]enable clk_infra_conn_main\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
	} else {
#if defined(CONFIG_MTK_CLKMGR)
		disable_clock(MT_CG_INFRA_CONNMCU_BUS, "WMT_MOD");
#else
		clk_disable_unprepare(clk_infra_conn_main);
		WMT_PLAT_DBG_FUNC("[CCF] clk_disable_unprepare(clk_infra_conn_main) calling\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
	}
#endif
	return 0;
}

static INT32 polling_consys_chipid(VOID)
{
	UINT32 retry = 10;
	UINT32 consysHwChipId = 0;

#ifdef CONFIG_OF		/*use DT */
	/*12.poll CONNSYS CHIP ID until chipid is returned  0x18070008 */
	while (retry-- > 0) {
		consysHwChipId = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CHIP_ID_OFFSET);
		if (consysHwChipId == 0x0279) {
			WMT_PLAT_INFO_FUNC("retry(%d)consys chipId(0x%08x)\n", retry, consysHwChipId);
			break;
		}
		WMT_PLAT_INFO_FUNC("Read CONSYS chipId(0x%08x)", consysHwChipId);
		msleep(20);
	}

	if ((0 == retry) || (0 == consysHwChipId)) {
		WMT_PLAT_ERR_FUNC("Maybe has a consys power on issue,(0x%08x)\n", consysHwChipId);
		WMT_PLAT_ERR_FUNC("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET));
		WMT_PLAT_ERR_FUNC("reg dump:CONSYS_PWR_CONN_ACK_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET));
		WMT_PLAT_ERR_FUNC("reg dump:CONSYS_PWR_CONN_ACK_S_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET));
		WMT_PLAT_ERR_FUNC("reg dump:CONSYS_TOP1_PWR_CTRL_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET));
	}
#else
	/*12.poll CONNSYS CHIP ID until 6752 is returned 0x18070008 32'h6752 */
	while (retry-- > 0) {
		WMT_PLAT_DBG_FUNC("CONSYS_CHIP_ID_REG(0x%08x)", CONSYS_REG_READ(CONSYS_CHIP_ID_REG));
		consysHwChipId = CONSYS_REG_READ(CONSYS_CHIP_ID_REG);
		if (consysHwChipId == 0x0279) {
			WMT_PLAT_INFO_FUNC("retry(%d)consys chipId(0x%08x)\n", retry, consysHwChipId);
			break;
		}
		msleep(20);
	}

	if ((0 == retry) || (0 == consysHwChipId)) {
		WMT_PLAT_ERR_FUNC("Maybe has a consys power on issue,(0x%08x)\n", consysHwChipId);
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG));
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_PWR_CONN_ACK_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_REG));
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_PWR_CONN_ACK_S_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_S_REG));
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_TOP1_PWR_CTRL_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG));
	}
#endif
	return 0;
}

static VOID consys_acr_reg_setting(VOID)
{
	/*
	 *14.write 1 to conn_mcu_confg ACR[1] if real speed MBIST
	 *(default write "1") ACR 0x18070110[18] 1'b1
	 *if this bit is 0, HW will do memory auto test under low CPU frequence (26M Hz)
	 *if this bit is 0, HW will do memory auto test under high CPU frequence(138M Hz)
	 *inclulding low CPU frequence
	 */
#ifdef CONFIG_OF		/*use DT */
	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET,
			CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET) |
			CONSYS_MCU_CFG_ACR_MBIST_BIT);
#else
	CONSYS_REG_WRITE(CONSYS_MCU_CFG_ACR_REG,
			CONSYS_REG_READ(CONSYS_MCU_CFG_ACR_REG) | CONSYS_MCU_CFG_ACR_MBIST_BIT);
#endif
}

static VOID consys_afe_reg_setting(VOID)
{
#ifdef CONFIG_OF		/*use DT */
#if CONSYS_AFE_REG_SETTING
	UINT8 *consys_afe_reg_base = NULL;
	UINT8 i = 0;

	/*15.default no need,update ANA_WBG(AFE) CR if needed, CONSYS_AFE_REG */
	consys_afe_reg_base = ioremap_nocache(CONSYS_AFE_REG_BASE, 0x100);
#if defined(CONFIG_MTK_GPS_REGISTER_SETTING)
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_GPS_SINGLE_OFFSET,
			CONSYS_REG_READ(consys_afe_reg_base + CONSYS_AFE_REG_GPS_SINGLE_OFFSET) |
			CONSYS_AFE_REG_GPS_SINGLE_12_VALUE);
#endif
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_RCK_01_OFFSET,
			CONSYS_AFE_REG_DIG_RCK_01_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_GPS_01_OFFSET,
			CONSYS_AFE_REG_WBG_RX_01_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_GPS_02_OFFSET,
			CONSYS_AFE_REG_WBG_RX_02_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_BT_RX_01_OFFSET,
			CONSYS_AFE_REG_WBG_RX_01_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_BT_RX_02_OFFSET,
			CONSYS_AFE_REG_WBG_RX_02_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_BT_TX_01_OFFSET,
			CONSYS_AFE_REG_WBG_TX_01_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_BT_TX_02_OFFSET,
			CONSYS_AFE_REG_WBG_TX_02_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_BT_TX_03_OFFSET,
			CONSYS_AFE_REG_WBG_TX_03_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_WF_RX_02_OFFSET,
			CONSYS_AFE_REG_WBG_RX_02_VALUE);
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_WF_TX_01_OFFSET,
			CONSYS_AFE_REG_WBG_TX_01_VALUE);
	/* CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_WF_TX_02_OFFSET,
			CONSYS_AFE_REG_WBG_TX_02_VALUE); */
	CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_WF_TX_03_OFFSET,
			CONSYS_AFE_REG_WBG_TX_03_VALUE);
#if 1
	WMT_PLAT_DBG_FUNC("Dump AFE register\n");
	for (i = 0; i < 64; i++) {
		WMT_PLAT_DBG_FUNC("reg:0x%08x|val:0x%08x\n",
				CONSYS_AFE_REG_BASE + 4*i, CONSYS_REG_READ(consys_afe_reg_base + 4*i));
	}
#endif
#endif
#endif
}

static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*need PMIC driver provide new API protocol */
		/*1.AP power on VCN_1V8 LDO (with PMIC_WRAP API) VCN_1V8  */
#if defined(CONFIG_MTK_PMIC_LEGACY)
		pmic_set_register_value(MT6351_PMIC_RG_VCN18_ON_CTRL, 0);
		hwPowerOn(MT6351_POWER_LDO_VCN18, VOL_1800, "wcn_drv");
#else
		if (reg_VCN18) {
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			if (regulator_enable(reg_VCN18))
				WMT_PLAT_ERR_FUNC("enable VCN18 fail\n");
			else
				WMT_PLAT_DBG_FUNC("enable VCN18 ok\n");
		}
#endif
	} else {
#if defined(CONFIG_MTK_PMIC_LEGACY)
		/*AP power off MT6625L VCN_1V8 LDO */
		pmic_set_register_value(MT6351_PMIC_RG_VCN18_ON_CTRL, 0);
		hwPowerDown(MT6351_POWER_LDO_VCN18, "wcn_drv");
#else
		if (reg_VCN18) {
			if (regulator_disable(reg_VCN18))
				WMT_PLAT_ERR_FUNC("disable VCN_1V8 fail!\n");
			else
				WMT_PLAT_DBG_FUNC("disable VCN_1V8 ok\n");
		}
#endif
	}
#endif
	return 0;
}

static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*switch VCN28 to HW control mode */
		pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 1);
	} else
		pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 0);
#endif
}

static INT32 consys_hw_vcn28_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*in co-clock mode,need to turn on vcn28 when fm on */
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN28, VOL_2800, "wcn_drv");
#else
		if (reg_VCN28) {
			regulator_set_voltage(reg_VCN28, 2800000, 2800000);
			if (regulator_enable(reg_VCN28))
				WMT_PLAT_ERR_FUNC("WMT do VCN28 PMIC on fail!\n");
		}
#endif
		WMT_PLAT_INFO_FUNC("turn on vcn28 for fm/gps usage in co-clock mode\n");
	} else {
		/*in co-clock mode,need to turn off vcn28 when fm off */
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerDown(MT6351_POWER_LDO_VCN28, "wcn_drv");
#else
		if (reg_VCN28)
			regulator_disable(reg_VCN28);
#endif
		WMT_PLAT_INFO_FUNC("turn off vcn28 for fm/gps usage in co-clock mode\n");
	}
#endif
	return 0;
}

static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_BT_WIFI_SHARE_V33
	/* spin_lock_irqsave(&gBtWifiV33.lock,gBtWifiV33.flags); */
	if (enable) {
		if (1 == gBtWifiV33.counter) {
			gBtWifiV33.counter++;
			WMT_PLAT_DBG_FUNC("V33 has been enabled,counter(%d)\n", gBtWifiV33.counter);
		} else if (2 == gBtWifiV33.counter) {
			WMT_PLAT_DBG_FUNC("V33 has been enabled,counter(%d)\n", gBtWifiV33.counter);
		} else {
#if CONSYS_PMIC_CTRL_ENABLE
			/*do BT PMIC on,depenency PMIC API ready */
			/*switch BT PALDO control from SW mode to HW mode:0x416[5]-->0x1 */
			/* VOL_DEFAULT, VOL_3300, VOL_3400, VOL_3500, VOL_3600 */
			hwPowerOn(MT6351_POWER_LDO_VCN33_BT, VOL_3300, "wcn_drv");
			mt6351_upmu_set_rg_vcn33_on_ctrl(1);
#endif
			WMT_PLAT_INFO_FUNC("WMT do BT/WIFI v3.3 on\n");
			gBtWifiV33.counter++;
		}

	} else {
		if (1 == gBtWifiV33.counter) {
			/*do BT PMIC off */
			/*switch BT PALDO control from HW mode to SW mode:0x416[5]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
			mt6351_upmu_set_rg_vcn33_on_ctrl(0);
			hwPowerDown(MT6351_POWER_LDO_VCN33_BT, "wcn_drv");
#endif
			WMT_PLAT_INFO_FUNC("WMT do BT/WIFI v3.3 off\n");
			gBtWifiV33.counter--;
		} else if (2 == gBtWifiV33.counter) {
			gBtWifiV33.counter--;
			WMT_PLAT_DBG_FUNC("V33 no need disabled,counter(%d)\n", gBtWifiV33.counter);
		} else {
			WMT_PLAT_DBG_FUNC("V33 has been disabled,counter(%d)\n", gBtWifiV33.counter);
		}

	}
	/* spin_unlock_irqrestore(&gBtWifiV33.lock,gBtWifiV33.flags); */
#else
	if (enable) {
		/*do BT PMIC on,depenency PMIC API ready */
		/*switch BT PALDO control from SW mode to HW mode:0x416[5]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
		/* VOL_DEFAULT, VOL_3300, VOL_3400, VOL_3500, VOL_3600 */
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN33_BT, VOL_3300, "wcn_drv");
		pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 1);
#else
		if (reg_VCN33_BT) {
			regulator_set_voltage(reg_VCN33_BT, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_BT))
				WMT_PLAT_ERR_FUNC("WMT do BT PMIC on fail!\n");
		}
#endif

#endif
		WMT_PLAT_INFO_FUNC("WMT do BT PMIC on\n");
	} else {
		/*do BT PMIC off */
		/*switch BT PALDO control from HW mode to SW mode:0x416[5]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_PMIC_LEGACY)
		pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 0);
		hwPowerDown(MT6351_POWER_LDO_VCN33_BT, "wcn_drv");
#else
		if (reg_VCN33_BT)
			regulator_disable(reg_VCN33_BT);
#endif
#endif
		WMT_PLAT_INFO_FUNC("WMT do BT PMIC off\n");
	}
#endif
	return 0;
}

static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_BT_WIFI_SHARE_V33
	mtk_wcn_consys_hw_bt_paldo_ctrl(enable);
#else
	if (enable) {
		/*do WIFI PMIC on,depenency PMIC API ready */
		/*switch WIFI PALDO control from SW mode to HW mode:0x418[14]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN33_WIFI, VOL_3300, "wcn_drv");
		pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_WIFI, 1);
#else
		if (reg_VCN33_WIFI) {
			regulator_set_voltage(reg_VCN33_WIFI, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_WIFI))
				WMT_PLAT_ERR_FUNC("WMT do WIFI PMIC on fail!\n");
		}
#endif
#endif
		WMT_PLAT_INFO_FUNC("WMT do WIFI PMIC on\n");
	} else {
		/*do WIFI PMIC off */
		/*switch WIFI PALDO control from HW mode to SW mode:0x418[14]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_PMIC_LEGACY)
		pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_WIFI, 0);
		hwPowerDown(MT6351_POWER_LDO_VCN33_WIFI, "wcn_drv");
#else
		if (reg_VCN33_WIFI)
			regulator_disable(reg_VCN33_WIFI);
#endif

#endif
		WMT_PLAT_INFO_FUNC("WMT do WIFI PMIC off\n");
	}
#endif
	return 0;
}

static INT32 consys_emi_mpu_set_region_protection(VOID)
{
#if CONSYS_EMI_MPU_SETTING
	/*set MPU for EMI share Memory */
	WMT_PLAT_INFO_FUNC("setting MPU for EMI share memory\n");

#if defined(CONFIG_ARCH_MT6797)
	emi_mpu_set_region_protection(gConEmiPhyBase + SZ_1M / 2,
			gConEmiPhyBase + SZ_1M - 1,
			19,
			SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
				FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION));
#else
	WMT_PLAT_WARN_FUNC("not define platform config\n");
#endif
#endif
	return 0;
}

static UINT32 consys_emi_set_remapping_reg(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	UINT32 addrPhy = 0;

	/*consys to ap emi remapping register:10000320, cal remapping address */
	addrPhy = (gConEmiPhyBase & 0xFFF00000) >> 20;

	/*enable consys to ap emi remapping bit12 */
	addrPhy = addrPhy | 0x1000;

	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET) | addrPhy);

	WMT_PLAT_INFO_FUNC("CONSYS_EMI_MAPPING dump in restore cb(0x%08x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET));
#endif
	return 0;
}

static INT32 bt_wifi_share_v33_spin_lock_init(VOID)
{
#if CONSYS_BT_WIFI_SHARE_V33
	gBtWifiV33.counter = 0;
	spin_lock_init(&gBtWifiV33.lock);
#endif
	return 0;
}

static INT32 consys_read_irq_info_from_dts(INT32 *irq_num, UINT32 *irq_flag)
{
#ifdef CONFIG_OF		/*use DT */
	struct device_node *node;
	UINT32 irq_info[3] = { 0, 0, 0 };

	INT32 iret = -1;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6797-consys");
	if (node) {
		*irq_num = irq_of_parse_and_map(node, 0);
		/* get the interrupt line behaviour */
		if (of_property_read_u32_array(node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			WMT_PLAT_ERR_FUNC("get irq flags from DTS fail!!\n");
			return iret;
		}
		*irq_flag = irq_info[2];
		WMT_PLAT_INFO_FUNC("get irq id(%d) and irq trigger flag(%d) from DT\n", *irq_num,
				   *irq_flag);
	} else {
		WMT_PLAT_ERR_FUNC("[%s] can't find CONSYS compatible node\n", __func__);
		return iret;
	}
#endif
	return 0;
}

static INT32 consys_read_reg_from_dts(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	INT32 iRet = -1;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6797-consys");
	if (node) {
		/* registers base address */
		conn_reg.mcu_base = (SIZE_T) of_iomap(node, 0);
		WMT_PLAT_DBG_FUNC("Get mcu register base(0x%zx)\n", conn_reg.mcu_base);
		conn_reg.ap_rgu_base = (SIZE_T) of_iomap(node, 1);
		WMT_PLAT_DBG_FUNC("Get ap_rgu register base(0x%zx)\n", conn_reg.ap_rgu_base);
		conn_reg.topckgen_base = (SIZE_T) of_iomap(node, 2);
		WMT_PLAT_DBG_FUNC("Get topckgen register base(0x%zx)\n", conn_reg.topckgen_base);
		conn_reg.spm_base = (SIZE_T) of_iomap(node, 3);
		WMT_PLAT_DBG_FUNC("Get spm register base(0x%zx)\n", conn_reg.spm_base);
	} else {
		WMT_PLAT_ERR_FUNC("[%s] can't find CONSYS compatible node\n", __func__);
		return iRet;
	}
#endif
	return 0;
}

static VOID force_trigger_assert_debug_pin(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) & ~CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_INFO_FUNC("enable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
	usleep_range(64, 96);
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) | CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_INFO_FUNC("disable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
#endif
}

static UINT32 consys_read_cpupcr(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CPUPCR_OFFSET);
#endif
}

static UINT32 consys_soc_chipid_get(void)
{
	return PLATFORM_SOC_CHIP;
}

static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID)
{
	return &mtk_wcn_emi_addr_info;
}

P_WMT_CONSYS_IC_OPS mtk_wcn_get_consys_ic_ops(VOID)
{
	return &consys_ic_ops;
}
