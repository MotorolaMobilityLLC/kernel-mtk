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
static INT32 mtk_wmt_probe(struct platform_device *pdev);
static INT32 mtk_wmt_remove(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
UINT8 __iomem *pEmibaseaddr = NULL;
phys_addr_t gConEmiPhyBase;
struct CONSYS_BASE_ADDRESS conn_reg;

/* CCF part */
#if !defined(CONFIG_MTK_CLKMGR)
struct clk *clk_scp_conn_main;	/*ctrl conn_power_on/off */
#if CONSYS_AHB_CLK_MAGEMENT
struct clk *clk_infra_conn_main;	/*ctrl infra_connmcu_bus clk */
#endif
#endif /* !defined(CONFIG_MTK_CLKMGR) */

#ifdef CONFIG_OF
static const struct of_device_id apwmt_of_ids[] = {
	{.compatible = "mediatek,mt6797-consys",},
	{}
};
#endif

static struct platform_driver mtk_wmt_dev_drv = {
	.probe = mtk_wmt_probe,
	.remove = mtk_wmt_remove,
	.driver = {
		   .name = "mtk_wmt",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = apwmt_of_ids,
#endif
		   },
};

/* PMIC part */
#if CONSYS_PMIC_CTRL_ENABLE
#if !defined(CONFIG_MTK_PMIC_LEGACY)
struct regulator *reg_VCN18;
struct regulator *reg_VCN28;
struct regulator *reg_VCN33_BT;
struct regulator *reg_VCN33_WIFI;
#endif
#endif

/* GPIO part */
#if !defined(CONFIG_MTK_GPIO_LEGACY)
struct pinctrl *consys_pinctrl = NULL;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#define DYNAMIC_DUMP_GROUP_NUM 5
#if CONSYS_ENALBE_SET_JTAG
UINT32 gJtagCtrl = 0;

#define JTAG_ADDR1_BASE 0x10005000
#define JTAG_ADDR2_BASE 0x10002000

char *jtag_addr1 = (char *)JTAG_ADDR1_BASE;
char *jtag_addr2 = (char *)JTAG_ADDR2_BASE;

#define JTAG1_REG_WRITE(addr, value)	\
writel(value, ((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))
#define JTAG1_REG_READ(addr)			\
readl(((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))

#define JTAG2_REG_WRITE(addr, value)	\
	writel(value, ((PUINT32)(jtag_addr2+(addr-JTAG_ADDR2_BASE))))
#define JTAG2_REG_READ(addr)			\
	readl(((PUINT32)(jtag_addr2+(addr-JTAG_ADDR2_BASE))))

static INT32 mtk_wcn_consys_jtag_set_for_mcu(VOID)
{
#if 1
	INT32 iRet = -1;
	UINT32 tmp = 0;

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

#endif
	return 0;
}

UINT32 mtk_wcn_consys_jtag_flag_ctrl(UINT32 en)
{
	WMT_PLAT_INFO_FUNC("%s jtag set for MCU\n", en ? "enable" : "disable");
	gJtagCtrl = en;
	return 0;
}

#endif

static INT32 mtk_wmt_probe(struct platform_device *pdev)
{
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

#if !defined(CONFIG_MTK_GPIO_LEGACY)
	consys_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(consys_pinctrl)) {
		WMT_PLAT_ERR_FUNC("cannot find consys pinctrl.\n");
		return PTR_ERR(consys_pinctrl);
	}
#endif /* !defined(CONFIG_MTK_GPIO_LEGACY) */

	return 0;
}

static INT32 mtk_wmt_remove(struct platform_device *pdev)
{
	return 0;
}

INT32 mtk_wcn_consys_hw_reg_ctrl(UINT32 on, UINT32 co_clock_type)
{
	INT32 iRet = -1;
	UINT32 retry = 10;
	UINT32 consysHwChipId = 0;
#if CONSYS_AFE_REG_SETTING
	UINT8 *consys_afe_reg_base = NULL;
	UINT8 i = 0;
#endif

	WMT_PLAT_DBG_FUNC("CONSYS-HW-REG-CTRL(0x%08x),start\n", on);

	if (on) {
		WMT_PLAT_DBG_FUNC("++\n");
/*step1.PMIC ctrl*/
#if CONSYS_PMIC_CTRL_ENABLE
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
		udelay(240);

		if (0 == co_clock_type) {
			/*switch VCN28 to HW control mode */
			pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 1);
			/*turn on VCN28 LDO */
#if defined(CONFIG_MTK_PMIC_LEGACY)
			hwPowerOn(MT6351_POWER_LDO_VCN28, VOL_2800, "wcn_drv");
#else
			if (reg_VCN28) {
				regulator_set_voltage(reg_VCN28, 2800000, 2800000);
				if (regulator_enable(reg_VCN28))
					WMT_PLAT_INFO_FUNC("enable VCN_2V8 fail!\n");
				else
					WMT_PLAT_INFO_FUNC("enable VCN_2V8 ok\n");
			}
#endif
		}
#endif
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_CONN2AP_SLEEP_MASK_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_CONN2AP_SLEEP_MASK_OFFSET) |
				 CONSYS_CONN2AP_SLEEP_MASK_BIT);
/*step2.MTCMOS ctrl*/

#ifdef CONFIG_OF		/*use DT */
		/*3.assert CONNSYS CPU SW reset  0x10007018 "[12]=1'b1  [31:24]=8'h88 (key)" */
		/*CONSYS_REG_WRITE((conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET),
				 CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) |
				 CONSYS_CPU_SW_RST_BIT | CONSYS_CPU_SW_RST_CTRL_KEY);*/
		mtk_wdt_swsysret_config((1 << 12), 1);
		WMT_PLAT_INFO_FUNC("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				  CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET));
		/*turn on SPM clock gating enable PWRON_CONFG_EN  0x10006000  32'h0b160001 */
		CONSYS_REG_WRITE((conn_reg.spm_base + CONSYS_PWRON_CONFG_EN_OFFSET), CONSYS_PWRON_CONFG_EN_VALUE);

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
#endif
		WMT_PLAT_DBG_FUNC("reg dump:CONSYS_PWR_CONN_ACK_REG(0x%x)\n",
				   CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET));
		WMT_PLAT_DBG_FUNC("reg dump:CONSYS_PWR_CONN_ACK_S_REG(0x%x)\n",
				   CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET));
		WMT_PLAT_DBG_FUNC("reg dump:CONSYS_TOP1_PWR_CTRL_REG(0x%x)\n",
				   CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET));
		/*11.26M is ready now, delay 10us for mem_pd de-assert */
		udelay(30);
		/*enable AP bus clock : connmcu_bus_pd  API: enable_clock() ++?? */
#if CONSYS_AHB_CLK_MAGEMENT
#if defined(CONFIG_MTK_CLKMGR)
		enable_clock(MT_CG_INFRA_CONNMCU_BUS, "WCN_MOD");
		WMT_PLAT_DBG_FUNC("enable MT_CG_INFRA_CONNMCU_BUS CLK\n");
#else
		iRet = clk_prepare_enable(clk_infra_conn_main);
		if (iRet)
			WMT_PLAT_ERR_FUNC("clk_prepare_enable(clk_infra_conn_main) fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("[CCF]enable clk_infra_conn_main\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
#endif
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

		/*13.{default no need}update ROMDEL/PATCH RAM DELSEL if needed 0x18070114 */

		/*
		 *14.write 1 to conn_mcu_confg ACR[1] if real speed MBIST
		 *(default write "1") ACR 0x18070110[18] 1'b1
		 *if this bit is 0, HW will do memory auto test under low CPU frequence (26M Hz)
		 *if this bit is 0, HW will do memory auto test under high CPU frequence(138M Hz)
		 *inclulding low CPU frequence
		 */
		CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET,
				 CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET) |
				 CONSYS_MCU_CFG_ACR_MBIST_BIT);

#if CONSYS_AFE_REG_SETTING
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
		 * CONSYS_AFE_REG_WBG_TX_02_VALUE); */
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

		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88 (key)" */
		/*CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				 (CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 ~CONSYS_CPU_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);*/
		mtk_wdt_swsysret_config((1 << 12), 0);
#else /*use HADRCODE, maybe no use.. */
		/*3.assert CONNSYS CPU SW reset  0x10007018  "[12]=1'b1  [31:24]=8'h88 (key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				 (CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) | CONSYS_CPU_SW_RST_BIT |
				  CONSYS_CPU_SW_RST_CTRL_KEY));
		/*turn on SPM clock gating enable PWRON_CONFG_EN 0x10006000 32'h0b160001 */
		CONSYS_REG_WRITE(CONSYS_PWRON_CONFG_EN_REG, CONSYS_PWRON_CONFG_EN_VALUE);

#if CONSYS_PWR_ON_OFF_API_AVAILABLE
#if defined(CONFIG_MTK_CLKMGR)
		iRet = conn_power_on();	/* consult clkmgr owner */
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
#endif
		/*11.26M is ready now, delay 10us for mem_pd de-assert */
		udelay(10);
		/*enable AP bus clock : connmcu_bus_pd  API: enable_clock() ++?? */
#if CONSYS_AHB_CLK_MAGEMENT
#if defined(CONFIG_MTK_CLKMGR)
		enable_clock(MT_CG_INFRA_CONNMCU_BUS, "WCN_MOD");
		WMT_PLAT_DBG_FUNC("enable MT_CG_INFRA_CONNMCU_BUS CLK\n");
#else
		iRet = clk_prepare_enable(clk_infra_conn_main);
		if (iRet)
			WMT_PLAT_ERR_FUNC("clk_prepare_enable(clk_infra_conn_main) fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("[CCF]enable clk_infra_conn_main\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
#endif
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

		/*13.{default no need}update ROMDEL/PATCH RAM DELSEL if needed 0x18070114  */

		/*
		 *14.write 1 to conn_mcu_confg ACR[1] if real speed MBIST
		 *(default write "1") ACR 0x18070110[18] 1'b1
		 *if this bit is 0, HW will do memory auto test under low CPU frequence (26M Hz)
		 *if this bit is 0, HW will do memory auto test under high CPU frequence(138M Hz)
		 *inclulding low CPU frequence
		 */
		CONSYS_REG_WRITE(CONSYS_MCU_CFG_ACR_REG,
				 CONSYS_REG_READ(CONSYS_MCU_CFG_ACR_REG) | CONSYS_MCU_CFG_ACR_MBIST_BIT);

		/*update ANA_WBG(AFE) CR. AFE setting file:  AP Offset = 0x180B2000   */

#if 0
		/*15.default no need,update ANA_WBG(AFE) CR if needed, CONSYS_AFE_REG */
		CONSYS_REG_WRITE(CONSYS_AFE_REG_DIG_RCK_01, CONSYS_AFE_REG_DIG_RCK_01_VALUE);
		CONSYS_REG_WRITE(CONSYS_AFE_REG_WBG_PLL_02, CONSYS_AFE_REG_WBG_PLL_02_VALUE);
		CONSYS_REG_WRITE(CONSYS_AFE_REG_WBG_WB_TX_01, CONSYS_AFE_REG_WBG_WB_TX_01_VALUE);
#endif
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88(key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				 (CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) & ~CONSYS_CPU_SW_RST_BIT) |
				 CONSYS_CPU_SW_RST_CTRL_KEY);

#endif
		msleep(20);

	} else {

#ifdef CONFIG_OF

#if CONSYS_AHB_CLK_MAGEMENT
#if defined(CONFIG_MTK_CLKMGR)
		disable_clock(MT_CG_INFRA_CONNMCU_BUS, "WMT_MOD");
#else
		clk_disable_unprepare(clk_infra_conn_main);
		WMT_PLAT_DBG_FUNC("[CCF] clk_disable_unprepare(clk_infra_conn_main) calling\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
#endif

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

#endif

#else

#if CONSYS_PWR_ON_OFF_API_AVAILABLE

#if CONSYS_AHB_CLK_MAGEMENT
#if defined(CONFIG_MTK_CLKMGR)
		disable_clock(MT_CG_INFRA_CONNMCU_BUS, "WMT_MOD");
#else
		clk_disable_unprepare(clk_infra_conn_main);
#endif /* defined(CONFIG_MTK_CLKMGR) */
#endif

#if defined(CONFIG_MTK_CLKMGR)
		/*power off connsys by API: conn_power_off() */
		iRet = conn_power_off();	/* consult clkmgr owner */
		if (iRet)
			WMT_PLAT_ERR_FUNC("conn_power_off fail(%d)\n", iRet);
		WMT_PLAT_DBG_FUNC("conn_power_off ok\n");
#else
		clk_disable_unprepare(clk_scp_conn_main);
		WMT_PLAT_DBG_FUNC("clk_disable_unprepare(clk_scp_conn_main) calling\n");
#endif /* defined(CONFIG_MTK_CLKMGR) */
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
#endif

#endif

#if CONSYS_PMIC_CTRL_ENABLE
			pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 0);
#if defined(CONFIG_MTK_PMIC_LEGACY)
			/*turn off VCN28 LDO (with PMIC_WRAP API)" */
			hwPowerDown(MT6351_POWER_LDO_VCN28, "wcn_drv");
#else
			if (reg_VCN28) {
				if (regulator_disable(reg_VCN28))
					WMT_PLAT_ERR_FUNC("disable VCN_2V8 fail!\n");
				else
					WMT_PLAT_DBG_FUNC("disable VCN_2V8 ok\n");
			}
#endif

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

#endif

	}
	WMT_PLAT_WARN_FUNC("CONSYS-HW-REG-CTRL(0x%08x),finish\n", on);
	return 0;
}

INT32 mtk_wcn_consys_hw_gpio_ctrl(UINT32 on)
{
	INT32 iRet = 0;

	WMT_PLAT_DBG_FUNC("CONSYS-HW-GPIO-CTRL(0x%08x), start\n", on);

	if (on) {

		/*if external modem used,GPS_SYNC still needed to control */
		iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_INIT);
		iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_INIT);

		iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_INIT);

		/* TODO: [FixMe][GeorgeKuo] double check if BGF_INT is implemented ok */
		/* iRet += wmt_plat_gpio_ctrl(PIN_BGF_EINT, PIN_STA_MUX); */
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_INIT);
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_EINT_DIS);
		WMT_PLAT_DBG_FUNC("CONSYS-HW, BGF IRQ registered and disabled\n");

	} else {

		/* set bgf eint/all eint to deinit state, namely input low state */
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_EINT_DIS);
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_DEINIT);
		WMT_PLAT_DBG_FUNC("CONSYS-HW, BGF IRQ unregistered and disabled\n");
		/* iRet += wmt_plat_gpio_ctrl(PIN_BGF_EINT, PIN_STA_DEINIT); */

		/*if external modem used,GPS_SYNC still needed to control */
		iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_DEINIT);
		iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_DEINIT);
		/* deinit gps_lna */
		iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_DEINIT);

	}
	WMT_PLAT_DBG_FUNC("CONSYS-HW-GPIO-CTRL(0x%08x), finish\n", on);
	return iRet;

}

INT32 mtk_wcn_consys_hw_pwr_on(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_DBG_FUNC("CONSYS-HW-PWR-ON, start\n");
	if (!gConEmiPhyBase) {
		WMT_PLAT_ERR_FUNC("EMI base address is invalid, CONNSYS can not be powered on!");
		WMT_PLAT_ERR_FUNC("To avoid the occurrence of KE!\n");
		return -1;
	}
	iRet += mtk_wcn_consys_hw_reg_ctrl(1, co_clock_type);
	iRet += mtk_wcn_consys_hw_gpio_ctrl(1);
#if CONSYS_ENALBE_SET_JTAG
	if (gJtagCtrl)
		mtk_wcn_consys_jtag_set_for_mcu();
#endif
	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-ON, finish(%d)\n", iRet);
	return iRet;
}

INT32 mtk_wcn_consys_hw_pwr_off(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_DBG_FUNC("CONSYS-HW-PWR-OFF, start\n");

	iRet += mtk_wcn_consys_hw_reg_ctrl(0, 0);
	iRet += mtk_wcn_consys_hw_gpio_ctrl(0);

	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-OFF, finish(%d)\n", iRet);
	return iRet;
}

INT32 mtk_wcn_consys_hw_rst(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_INFO_FUNC("CONSYS-HW, hw_rst start, eirq should be disabled before this step\n");

	/*1. do whole hw power off flow */
	iRet += mtk_wcn_consys_hw_reg_ctrl(0, co_clock_type);

	/*2. do whole hw power on flow */
	iRet += mtk_wcn_consys_hw_reg_ctrl(1, co_clock_type);

	WMT_PLAT_INFO_FUNC("CONSYS-HW, hw_rst finish, eirq should be enabled after this step\n");
	return iRet;
}

#if CONSYS_BT_WIFI_SHARE_V33
INT32 mtk_wcn_consys_hw_bt_paldo_ctrl(UINT32 enable)
{
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
	return 0;
}

INT32 mtk_wcn_consys_hw_wifi_paldo_ctrl(UINT32 enable)
{
	mtk_wcn_consys_hw_bt_paldo_ctrl(enable);
	return 0;
}

#else
INT32 mtk_wcn_consys_hw_bt_paldo_ctrl(UINT32 enable)
{

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

	return 0;

}

INT32 mtk_wcn_consys_hw_wifi_paldo_ctrl(UINT32 enable)
{

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

	return 0;

}

#endif
INT32 mtk_wcn_consys_hw_vcn28_ctrl(UINT32 enable)
{
	if (enable) {
		/*in co-clock mode,need to turn on vcn28 when fm on */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN28, VOL_2800, "wcn_drv");
#else
		if (reg_VCN28) {
			regulator_set_voltage(reg_VCN28, 2800000, 2800000);
			if (regulator_enable(reg_VCN28))
				WMT_PLAT_ERR_FUNC("WMT do VCN28 PMIC on fail!\n");
		}
#endif
#endif
		WMT_PLAT_INFO_FUNC("turn on vcn28 for fm/gps usage in co-clock mode\n");
	} else {
		/*in co-clock mode,need to turn off vcn28 when fm off */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_PMIC_LEGACY)
		hwPowerDown(MT6351_POWER_LDO_VCN28, "wcn_drv");
#else
		if (reg_VCN28)
			regulator_disable(reg_VCN28);
#endif
#endif
		WMT_PLAT_INFO_FUNC("turn off vcn28 for fm/gps usage in co-clock mode\n");
	}
	return 0;
}

INT32 mtk_wcn_consys_hw_state_show(VOID)
{
	return 0;
}

INT32 mtk_wcn_consys_hw_restore(struct device *device)
{
	UINT32 addrPhy = 0;

	if (gConEmiPhyBase) {

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
		/*consys to ap emi remapping register:10000320, cal remapping address */
		addrPhy = (gConEmiPhyBase & 0xFFF00000) >> 20;

		/*enable consys to ap emi remapping bit12 */
		addrPhy = addrPhy | 0x1000;

		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET) | addrPhy);

		WMT_PLAT_INFO_FUNC("CONSYS_EMI_MAPPING dump in restore cb(0x%08x)\n",
				   CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET));

#if 1
		pEmibaseaddr = ioremap_nocache(gConEmiPhyBase + SZ_1M / 2, CONSYS_EMI_MEM_SIZE);
#else
		pEmibaseaddr = ioremap_nocache(CONSYS_EMI_AP_PHY_BASE, CONSYS_EMI_MEM_SIZE);
#endif
		if (pEmibaseaddr) {
			WMT_PLAT_INFO_FUNC("EMI mapping OK(0x%p)\n", pEmibaseaddr);
			memset_io(pEmibaseaddr, 0, CONSYS_EMI_MEM_SIZE);
		} else {
			WMT_PLAT_ERR_FUNC("EMI mapping fail\n");
		}
	} else {
		WMT_PLAT_ERR_FUNC("consys emi memory address gConEmiPhyBase invalid\n");
	}

	return 0;
}

/*Reserved memory by device tree!*/
int reserve_memory_consys_fn(struct reserved_mem *rmem)
{
	WMT_PLAT_WARN_FUNC(" name: %s, base: 0x%llx, size: 0x%llx\n", rmem->name,
			   (unsigned long long)rmem->base, (unsigned long long)rmem->size);
	gConEmiPhyBase = rmem->base;
	return 0;
}
RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,consys-reserve-memory", reserve_memory_consys_fn);


INT32 mtk_wcn_consys_hw_init(void)
{

	INT32 iRet = -1;
	UINT32 addrPhy = 0;
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

	if (gConEmiPhyBase) {
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
		WMT_PLAT_DBG_FUNC("get consys start phy address(0x%zx)\n", (SIZE_T) gConEmiPhyBase);

		/*consys to ap emi remapping register:10000320, cal remapping address */
		addrPhy = (gConEmiPhyBase & 0xFFF00000) >> 20;

		/*enable consys to ap emi remapping bit12 */
		addrPhy = addrPhy | 0x1000;

		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET) | addrPhy);

		WMT_PLAT_INFO_FUNC("CONSYS_EMI_MAPPING dump(0x%08x)\n",
				   CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET));

#if 1
		pEmibaseaddr = ioremap_nocache(gConEmiPhyBase + SZ_1M / 2, CONSYS_EMI_MEM_SIZE);
#else
		pEmibaseaddr = ioremap_nocache(CONSYS_EMI_AP_PHY_BASE, CONSYS_EMI_MEM_SIZE);
#endif
		/* pEmibaseaddr = ioremap_nocache(0x80090400,270*KBYTE); */
		if (pEmibaseaddr) {
			WMT_PLAT_INFO_FUNC("EMI mapping OK(0x%p)\n", pEmibaseaddr);
			memset_io(pEmibaseaddr, 0, CONSYS_EMI_MEM_SIZE);
			iRet = 0;
		} else {
			WMT_PLAT_ERR_FUNC("EMI mapping fail\n");
		}
	} else {
		WMT_PLAT_ERR_FUNC("consys emi memory address gConEmiPhyBase invalid\n");
	}
#ifdef CONFIG_MTK_HIBERNATION
	WMT_PLAT_INFO_FUNC("register connsys restore cb for complying with IPOH function\n");
	register_swsusp_restore_noirq_func(ID_M_CONNSYS, mtk_wcn_consys_hw_restore, NULL);
#endif

	iRet = platform_driver_register(&mtk_wmt_dev_drv);
	if (iRet)
		WMT_PLAT_ERR_FUNC("WMT platform driver registered failed(%d)\n", iRet);

	return iRet;

}

INT32 mtk_wcn_consys_hw_deinit(void)
{
	if (pEmibaseaddr) {
		iounmap(pEmibaseaddr);
		pEmibaseaddr = NULL;
	}
#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_CONNSYS);
#endif

	platform_driver_unregister(&mtk_wmt_dev_drv);
	return 0;
}

UINT8 *mtk_wcn_consys_emi_virt_addr_get(UINT32 ctrl_state_offset)
{
	UINT8 *p_virtual_addr = NULL;

	if (!pEmibaseaddr) {
		WMT_PLAT_ERR_FUNC("EMI base address is NULL\n");
		return NULL;
	}
	WMT_PLAT_DBG_FUNC("ctrl_state_offset(%08x)\n", ctrl_state_offset);
	p_virtual_addr = pEmibaseaddr + ctrl_state_offset;

	return p_virtual_addr;
}

UINT32 mtk_wcn_consys_soc_chipid(void)
{
	return PLATFORM_SOC_CHIP;
}

#if !defined(CONFIG_MTK_GPIO_LEGACY)
struct pinctrl *mtk_wcn_consys_get_pinctrl()
{
	return consys_pinctrl;
}
#endif
INT32 mtk_wcn_consys_set_dbg_mode(UINT32 flag)
{
	INT32 ret = -1;
	PUINT8 vir_addr = NULL;

	vir_addr = mtk_wcn_consys_emi_virt_addr_get(EXP_APMEM_CTRL_CHIP_FW_DBGLOG_MODE);
	if (!vir_addr) {
		WMT_PLAT_ERR_FUNC("get vir address fail\n");
		return -2;
	}
	if (flag) {
		ret = 0;
		CONSYS_REG_WRITE(vir_addr, 0x1);
	} else {
		CONSYS_REG_WRITE(vir_addr, 0x0);
	}
	WMT_PLAT_ERR_FUNC("fw dbg mode register value(0x%08x)\n", CONSYS_REG_READ(vir_addr));
	return ret;
}
INT32 mtk_wcn_consys_set_dynamic_dump(PUINT32 str_buf)
{
	PUINT8 vir_addr = NULL;

	vir_addr = mtk_wcn_consys_emi_virt_addr_get(EXP_APMEM_CTRL_CHIP_DYNAMIC_DUMP);
	if (!vir_addr) {
		WMT_PLAT_ERR_FUNC("get vir address fail\n");
		return -2;
	}
	memcpy(vir_addr, str_buf, DYNAMIC_DUMP_GROUP_NUM*8);
	WMT_PLAT_INFO_FUNC("dynamic dump register value(0x%08x)\n", CONSYS_REG_READ(vir_addr));
	return 0;
}
