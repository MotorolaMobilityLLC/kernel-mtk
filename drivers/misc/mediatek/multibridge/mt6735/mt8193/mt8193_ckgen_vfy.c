#if defined(CONFIG_MTK_MULTIBRIDGE_SUPPORT)

#define MT8193_CKGEN_VFY 1

#if MT8193_CKGEN_VFY

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include "mt8193.h"
#include "mt8193_ckgen.h"


int mt8193_ckgen_config_pad_level_shift(int i4GroupNum, int i4TurnLow)
{
	u32 u4Tmp;

	pr_debug("[CKGEN] mt8193_ckgen_config_pad_level_shift() %d, %d\n", i4GroupNum, i4TurnLow);

	if (i4TurnLow == 0) {
		/* 3.3V->1.8V */
		u4Tmp = CKGEN_READ32(REG_RW_LS_CTRL);
		u4Tmp |= LS_CTRL_SHIFT_LOW_EN;
		CKGEN_WRITE32(REG_RW_LS_CTRL, u4Tmp);
		u4Tmp &= (~(1U<<i4GroupNum));
		CKGEN_WRITE32(REG_RW_LS_CTRL, u4Tmp);
	} else {
		/* 1.8V -> 3.3V */
		u4Tmp = CKGEN_READ32(REG_RW_LS_CTRL);
		u4Tmp |= LS_CTRL_SHIFT_HIGH_EN;
		CKGEN_WRITE32(REG_RW_LS_CTRL, u4Tmp);
		u4Tmp |= (1U<<i4GroupNum);
		CKGEN_WRITE32(REG_RW_LS_CTRL, u4Tmp);
	}

	pr_debug("[CKGEN] LS_CTRL: 0x%x\n", u4Tmp);

	return 0;
}

u32 mt8193_ckgen_reg_rw_test(u16 addr)
{
	u32 u4Loop = 0;

	pr_debug("[CKGEN] mt8193_ckgen_reg_rw_test() 0x%x\n", addr);

	for (; u4Loop < 0xFFFF; u4Loop += 0x10) {
		CKGEN_WRITE32(addr, u4Loop);
		if (CKGEN_READ32(addr) != u4Loop) {
			pr_err("[CKGEN] reg rw test fail at loop 0x%x\n", u4Loop);
			return -1;
		}
	}

	pr_debug("[CKGEN] mt8193_ckgen_reg_rw_test() success at 0x%x\n", addr);

	return 0;
}


u32 mt8193_ckgen_bus_clk_switch_xtal_test(u16 addr)
{
	/* switch bus clock to 32k. pdwn dcxo. rw register */
	u32 u4Tmp = 0;

	/* swicth bus clock to 32k */
	u4Tmp = CKGEN_READ32(REG_RW_BUS_CKCFG);
	CKGEN_WRITE32(REG_RW_BUS_CKCFG, (u4Tmp & (~(0xF))) | CLK_BUS_SEL_32K);
	return 0;
}

u32 mt8193_io_agent_test(void)
{
	u32 u4Tmp = 0;

	pr_debug("[CKGEN] IO AGENT TEST ------------------------------------------\n");

	IO_WRITE32(0x1000, 0x500, 0x55555555);
	u4Tmp = IO_READ32(0x1000, 0x500);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x1500\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1500. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x0, 0x18, 0x55555555);
	u4Tmp = IO_READ32(0x0, 0x18);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x18\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x18. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x0, 0x0408, 0x55555555);
	u4Tmp = IO_READ32(0x0, 0x408);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x408\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x408. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x0, 0x0608, 0x55555555);
	u4Tmp = IO_READ32(0x0, 0x608);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x608\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x608. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x0, 0x0a10, 0x55555555);
	u4Tmp = IO_READ32(0x0, 0xa10);
	if (u4Tmp == 0x00055555)
		pr_debug("[CKGEN] TEST PASS at 0xa10\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0xa10. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x1000, 0x200, 0xaaaaffff);
	u4Tmp = IO_READ32(0x1000, 0x200);
	if (u4Tmp == 0xaaaaffff)
		pr_debug("[CKGEN] TEST PASS at 0x1200\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1200. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x1000, 0x608, 0x05550555);
	u4Tmp = IO_READ32(0x1000, 0x608);
	if (u4Tmp == 0x05550555)
		pr_debug("[CKGEN] TEST PASS at 0x1608\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1608. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x1000, 0x708, 0x55555555);
	u4Tmp = IO_READ32(0x1000, 0x708);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x1708\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1708. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x1000, 0xd00, 0x55555555);
	u4Tmp = IO_READ32(0x1000, 0xd00);
	if (u4Tmp == 0x55555555)
		pr_debug("[CKGEN] TEST PASS at 0x1d00\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1d00. [0x%x] !!!!!!!!!!\n", u4Tmp);

	IO_WRITE32(0x1000, 0xd00, 0xaaaaaaaa);
	u4Tmp = IO_READ32(0x1000, 0xd00);
	if (u4Tmp == 0xaaaaaaaa)
		pr_debug("[CKGEN] TEST PASS at 0x1d00\n");
	else
		pr_err("[CKGEN] TEST FAIL at 0x1d00. [0x%x] !!!!!!!!!!\n", u4Tmp);

	pr_debug("[CKGEN] IO AGENT TEST FINISH ------------------------------------------\n");

	return 0;
}

void mt8193_hdmi_on_test(void)
{
	int i = 0;
	u32 u4Crc = 0;

	mt8193_i2c_write(0x1254, 0x00000323);
	msleep(100);
	mt8193_i2c_write(0x101c, 0x00000004);
	msleep(100);
	mt8193_i2c_write(0x1328, 0x00009999);
	msleep(100);
	mt8193_i2c_write(0x1334, 0x0020008f);
	msleep(100);
	mt8193_i2c_write(0x1338, 0xd4a88f00);
	msleep(100);
	mt8193_i2c_write(0x1344, 0x00008012);
	msleep(100);
	mt8193_i2c_write(0x1348, 0x11ff0000);
	msleep(100);
	mt8193_i2c_write(0x1334, 0x0030008f);
	msleep(100);
	mt8193_i2c_write(0x02d4, 0x02);
	msleep(100);
	mt8193_i2c_write(0x02d8, 0x80);
	msleep(100);
	mt8193_i2c_write(0x0448, 0x00000);
	msleep(100);
	mt8193_i2c_write(0x0450, 0x2);
	msleep(100);
	mt8193_i2c_write(0x0608, 0x80000005);
	msleep(100);
	mt8193_i2c_write(0x065c, 0x0000000f);
	msleep(100);
	mt8193_i2c_write(0x0604, 0x00000040);
	msleep(100);
	mt8193_i2c_write(0x061c, 0x00104000);
	msleep(100);
	mt8193_i2c_write(0x0624, 0x02ee07bc);
	msleep(100);
	mt8193_i2c_write(0x0628, 0x00030005);
	msleep(100);
	mt8193_i2c_write(0x0630, 0x0080057f);
	msleep(100);
	mt8193_i2c_write(0x0634, 0x001002df);
	msleep(100);
	mt8193_i2c_write(0x0638, 0x001002df);
	msleep(100);
	mt8193_i2c_write(0x0620, 0x000207ba);
	msleep(100);
	mt8193_i2c_write(0x0600, 0x00000001);
	msleep(100);
	mt8193_i2c_write(0x060c, 0x00000002);
	msleep(100);
	mt8193_i2c_write(0x0700, 0x02ee07bc);
	msleep(100);
	mt8193_i2c_write(0x0704, 0x00030005);
	msleep(100);
	mt8193_i2c_write(0x0708, 0x01000080);
	msleep(100);
	mt8193_i2c_write(0x070c, 0x02d00500);
	msleep(100);
	mt8193_i2c_write(0x0710, 0x00000203);
	msleep(100);
	mt8193_i2c_write(0x0714, 0x00ff8844);
	msleep(100);
	mt8193_i2c_write(0x0718, 0x000000ff);
	msleep(100);

	for (; i < 100; i++) {
		u32 u4Crc_2 = 0;

		mt8193_i2c_write(0x071c, 2);
		msleep(50);
		mt8193_i2c_write(0x071c, 1);
		msleep(50);
		mt8193_i2c_read(0x0720, &u4Crc_2);
		msleep(50);

		if (i > 1 && u4Crc_2 != u4Crc) {
			pr_err("[HDMI] CHECK CRC ERROR! 0x%x, 0x%x, %d\n", u4Crc, u4Crc_2, i);
		break;
		} else {
			pr_debug("[HDMI] CHECK CRC OK %d\n", i);
		}

		u4Crc = u4Crc_2;
	}
}

void mt8193_spm_control_test(int u4Func)
{
	pr_debug("[CKGEN] mt8193_spm_control_test()enters. %d\n", u4Func);

	/* mt8193_io_agent_test(); */

	if (u4Func == 0) {
		/* loop 1 */
		mt8193_lvds_sys_spm_control(false);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(false);
		msleep(1000);
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);

		mt8193_lvds_sys_spm_control(true);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(true);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();

		/* loop 2 */
		mt8193_hdmi_sys_spm_control(false);
		msleep(1000);
		mt8193_lvds_sys_spm_control(false);
		msleep(1000);
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);

		mt8193_lvds_sys_spm_control(true);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(true);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();

		/* loop 3 */
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);
		mt8193_lvds_sys_spm_control(false);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(false);
		msleep(1000);

		mt8193_lvds_sys_spm_control(true);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(true);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();

		/* loop 4 */
		mt8193_lvds_sys_spm_control(false);
		msleep(1000);
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(false);
		msleep(1000);

		mt8193_lvds_sys_spm_control(true);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(true);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();

		/* loop 5 */

		mt8193_hdmi_sys_spm_control(false);
		msleep(1000);
		mt8193_lvds_sys_spm_control(false);
		msleep(1000);
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);

		mt8193_lvds_sys_spm_control(true);
		msleep(1000);
		mt8193_hdmi_sys_spm_control(true);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();
	} else if (u4Func == 1) {
		/*
		step 1: turn off digital
		step 2: turn off analog
		step3:  BUS CLK SWITCH TO 32K
		step4: PULL LOW EN_BB & CK_SEL
		Step5: PULL UP EN_BB & CK_SEL
		step6: BUS CLK SWITCH to 26M
		step7: turn on analog
		step8: turn on digital
		step9: test function
		*/
		/* lvds spm ctrl test */
		mt8193_lvds_sys_spm_control(false);
		msleep(100);
		/*mt8193_hdmi_sys_spm_control(false);*/
		msleep(100);
		/*mt8193_nfi_sys_spm_control(false);*/
		msleep(100);

		mt8193_lvds_ana_pwr_control(false);
		msleep(100);
		/*mt8193_hdmi_ana_pwr_control(false);*/
		msleep(100);
		mt8193_pllgp_ana_pwr_control(false);
		msleep(100);
		/*mt8193_nfi_ana_pwr_control(false);*/
		msleep(1000);

		/* bus clk switch to 32K*/
		mt8193_bus_clk_switch(true);

		msleep(20);
		/* pull low en_bb */
		/* mt8193_en_bb_ctrl(true); */

		msleep(100);

		/* pull up en_bb */
		/* mt8193_en_bb_ctrl(false); */

		/* msleep(20); */

		/* bus clk switch to 26M */
		mt8193_bus_clk_switch(false);

		/*mt8193_nfi_ana_pwr_control(true);*/
		msleep(100);
		mt8193_pllgp_ana_pwr_control(true);
		msleep(100);
		/*mt8193_hdmi_ana_pwr_control(true);*/
		msleep(100);
		mt8193_lvds_ana_pwr_control(true);
		msleep(100);
		mt8193_lvds_sys_spm_control(true);
		msleep(100);
		/*mt8193_hdmi_sys_spm_control(true);*/
		msleep(100);
		/*mt8193_nfi_sys_spm_control(true);*/
		msleep(100);

		/* lcm_mt8193_lvds_on_test();*/
	} else if (u4Func == 2) {
		/*int i = 0;*/

		/*for (; i<=1000; i++) { */
		/*pr_debug("[CKGEN] LOOP %d START-------------------------------------\n", i);*/
			/* hdmi spm ctrl test */
			msleep(1000);
			mt8193_lvds_ana_pwr_control(false);
			msleep(20);
			mt8193_hdmi_ana_pwr_control(false);
			msleep(20);
			mt8193_pllgp_ana_pwr_control(false);
			msleep(20);
			mt8193_nfi_ana_pwr_control(false);
			msleep(20);
			mt8193_lvds_sys_spm_control(false);
			msleep(20);
			mt8193_hdmi_sys_spm_control(false);
			msleep(20);
			mt8193_nfi_sys_spm_control(false);
			msleep(20);

			/* bus clk switch to 32K */
			mt8193_bus_clk_switch(true);

			msleep(1000);
			/* pull low en_bb */
			/*mt8193_en_bb_ctrl(true);*/

			/*msleep(100);*/

			/* pull up en_bb*/
			/*mt8193_en_bb_ctrl(false);*/

			/*msleep(1);*/

			/* bus clk switch to 26M */
			mt8193_bus_clk_switch(false);
			msleep(20);
			mt8193_lvds_sys_spm_control(true);
			msleep(20);
			mt8193_hdmi_sys_spm_control(true);
			msleep(20);
			mt8193_nfi_sys_spm_control(true);
			msleep(20);
			mt8193_nfi_ana_pwr_control(true);
			msleep(20);
			mt8193_pllgp_ana_pwr_control(true);
			msleep(20);
			mt8193_hdmi_ana_pwr_control(true);
			msleep(20);
			mt8193_lvds_ana_pwr_control(true);
			msleep(1000);

			/*pr_debug("[CKGEN] LOOP %d END-------------------------------------\n", i); */
		/*}*/

		/*mt8193_hdmi_on_test();*/
	} else if (u4Func == 3) {
		/* nfi spm ctrl test */
		mt8193_nfi_sys_spm_control(false);
		msleep(1000);
		mt8193_nfi_sys_spm_control(true);
		msleep(1000);

		mt8193_io_agent_test();
	} else if (u4Func == 4) {
		/* bus clk switch to 32K */
		mt8193_bus_clk_switch(true);
		msleep(20);
		/* pull low en_bb */
		/* mt8193_en_bb_ctrl(true); */
	} else if (u4Func == 5) {
		/* pull up en_bb */
		/* mt8193_en_bb_ctrl(false); */
		/* bus clk switch to 32K */
		mt8193_bus_clk_switch(false);
	}

	pr_debug("[CKGEN] mt8193_spm_control() exit\n");
}

#endif
#endif
