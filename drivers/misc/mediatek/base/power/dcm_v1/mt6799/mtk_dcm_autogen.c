/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_chip.h>
#include <mt-plat/mtk_secure_api.h>

#include <mtk_dcm_internal.h>
#include <mtk_dcm_autogen.h>
#include <mtk_dcm.h>

#define INFRACFG_AO_AXI_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4))
#define INFRACFG_AO_AXI_REG1_MASK ((0x1 << 0))
#define INFRACFG_AO_AXI_REG0_ON ((0x1 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4))
#define INFRACFG_AO_AXI_REG0_E2_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4))
#define INFRACFG_AO_AXI_REG1_ON ((0x0 << 0))
#define INFRACFG_AO_AXI_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4))
#define INFRACFG_AO_AXI_REG1_OFF ((0x1 << 0))
#if 0
static unsigned int infracfg_ao_infra_dcm_rg_fsel_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 5) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_dcm_rg_sfsel_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_dcm_dbc_rg_dbc_num_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 15) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_dcm_dbc_rg_dbc_en_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 20) & 0x1;
}
#endif
static void infracfg_ao_infra_dcm_rg_fsel_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 5)) |
		(val & 0x1f) << 5);
}
static void infracfg_ao_infra_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}
static void infracfg_ao_infra_dcm_dbc_rg_dbc_num_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 15)) |
		(val & 0x1f) << 15);
}
static void infracfg_ao_infra_dcm_dbc_rg_dbc_en_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1 << 20)) |
		(val & 0x1) << 20);
}

bool dcm_infracfg_ao_axi_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
		ret &= ((reg_read(INFRA_BUS_DCM_CTRL) &
			INFRACFG_AO_AXI_REG0_MASK) ==
			(unsigned int) INFRACFG_AO_AXI_REG0_E2_ON);
	else
		ret &= ((reg_read(INFRA_BUS_DCM_CTRL) &
			INFRACFG_AO_AXI_REG0_MASK) ==
			(unsigned int) INFRACFG_AO_AXI_REG0_ON);
	ret &= ((reg_read(INFRA_BUS_DCM_CTRL_1) &
		INFRACFG_AO_AXI_REG1_MASK) ==
		(unsigned int) INFRACFG_AO_AXI_REG1_ON);

	return ret;
}

void dcm_infracfg_ao_axi(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_axi'" */
		infracfg_ao_infra_dcm_rg_fsel_set(0x1f);
		infracfg_ao_infra_dcm_rg_sfsel_set(0x4);
		infracfg_ao_infra_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_dcm_dbc_rg_dbc_en_set(0x1);
		if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
			reg_write(INFRA_BUS_DCM_CTRL,
				(reg_read(INFRA_BUS_DCM_CTRL) &
				~INFRACFG_AO_AXI_REG0_MASK) |
				INFRACFG_AO_AXI_REG0_E2_ON);
		else
			reg_write(INFRA_BUS_DCM_CTRL,
				(reg_read(INFRA_BUS_DCM_CTRL) &
				~INFRACFG_AO_AXI_REG0_MASK) |
				INFRACFG_AO_AXI_REG0_ON);
		reg_write(INFRA_BUS_DCM_CTRL_1,
			(reg_read(INFRA_BUS_DCM_CTRL_1) &
			~INFRACFG_AO_AXI_REG1_MASK) |
			INFRACFG_AO_AXI_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_axi'" */
		infracfg_ao_infra_dcm_rg_fsel_set(0x1f);
		infracfg_ao_infra_dcm_rg_sfsel_set(0x4);
		infracfg_ao_infra_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_dcm_dbc_rg_dbc_en_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL,
			(reg_read(INFRA_BUS_DCM_CTRL) &
			~INFRACFG_AO_AXI_REG0_MASK) |
			INFRACFG_AO_AXI_REG0_OFF);
		reg_write(INFRA_BUS_DCM_CTRL_1,
			(reg_read(INFRA_BUS_DCM_CTRL_1) &
			~INFRACFG_AO_AXI_REG1_MASK) |
			INFRACFG_AO_AXI_REG1_OFF);
	}
}

#define INFRACFG_AO_EMI_REG0_MASK ((0x1 << 1) | \
			(0x3 << 2))
#define INFRACFG_AO_EMI_REG1_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x3 << 6) | \
			(0x3 << 8) | \
			(0x3 << 10) | \
			(0x3 << 12) | \
			(0x3 << 14))
#define INFRACFG_AO_EMI_REG0_ON ((0x0 << 1) | \
			(0x1 << 2))
#if 1 /* dcm_infracfg_ao_emi_indiv(on); */
#define INFRACFG_AO_EMI_REG1_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x3 << 6) | \
			(0x3 << 8) | \
			(0x3 << 10) | \
			(0x3 << 12) | \
			(0x3 << 14))
#else
#define INFRACFG_AO_EMI_REG1_ON ((0x1 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 8) | \
			(0x0 << 10) | \
			(0x0 << 12) | \
			(0x0 << 14))
#endif
#define INFRACFG_AO_EMI_REG0_OFF ((0x1 << 1) | \
			(0x0 << 2))
#define INFRACFG_AO_EMI_REG1_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 8) | \
			(0x0 << 10) | \
			(0x0 << 12) | \
			(0x0 << 14))
#if 0
static unsigned int infracfg_ao_rg_emi_idle_sel_to_top_onlye2_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL_1) >> 4) & 0x1;
}
#endif
static void infracfg_ao_rg_emi_idle_sel_to_top_onlye2_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL_1,
		(reg_read(INFRA_BUS_DCM_CTRL_1) &
		~(0x1 << 4)) |
		(val & 0x1) << 4);
}

bool dcm_infracfg_ao_emi_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(INFRA_BUS_DCM_CTRL_1) &
		INFRACFG_AO_EMI_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_EMI_REG0_ON);
	ret &= ((reg_read(INFRA_EMI_BUS_CTRL_1_STA) &
		INFRACFG_AO_EMI_REG1_MASK) ==
		(unsigned int) INFRACFG_AO_EMI_REG1_ON);

	return ret;
}

void dcm_infracfg_ao_emi(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_emi'" */
		infracfg_ao_rg_emi_idle_sel_to_top_onlye2_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL_1,
			(reg_read(INFRA_BUS_DCM_CTRL_1) &
			~INFRACFG_AO_EMI_REG0_MASK) |
			INFRACFG_AO_EMI_REG0_ON);
		reg_write(INFRA_EMI_DCM_CTRL,
			(reg_read(INFRA_EMI_BUS_CTRL_1_STA) &
			~INFRACFG_AO_EMI_REG1_MASK) |
			INFRACFG_AO_EMI_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_emi'" */
		infracfg_ao_rg_emi_idle_sel_to_top_onlye2_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL_1,
			(reg_read(INFRA_BUS_DCM_CTRL_1) &
			~INFRACFG_AO_EMI_REG0_MASK) |
			INFRACFG_AO_EMI_REG0_OFF);
		reg_write(INFRA_EMI_DCM_CTRL,
			(reg_read(INFRA_EMI_BUS_CTRL_1_STA) &
			~INFRACFG_AO_EMI_REG1_MASK) |
			INFRACFG_AO_EMI_REG1_OFF);
	}

	/* dcm_infracfg_ao_emi_indiv(on); */
}

#define INFRACFG_AO_MD_QAXI_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define INFRACFG_AO_MD_QAXI_REG0_ON ((0x1 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#define INFRACFG_AO_MD_QAXI_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#if 0
static unsigned int infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_num_get(void)
{
	return (reg_read(INFRA_MDBUS_DCM_CTRL) >> 15) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_en_get(void)
{
	return (reg_read(INFRA_MDBUS_DCM_CTRL) >> 20) & 0x1;
}
#endif
static void infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_num_set(unsigned int val)
{
	reg_write(INFRA_MDBUS_DCM_CTRL,
		(reg_read(INFRA_MDBUS_DCM_CTRL) &
		~(0x1f << 15)) |
		(val & 0x1f) << 15);
}
static void infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_en_set(unsigned int val)
{
	reg_write(INFRA_MDBUS_DCM_CTRL,
		(reg_read(INFRA_MDBUS_DCM_CTRL) &
		~(0x1 << 20)) |
		(val & 0x1) << 20);
}

bool dcm_infracfg_ao_md_qaxi_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(INFRA_MDBUS_DCM_CTRL) &
		INFRACFG_AO_MD_QAXI_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_MD_QAXI_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_md_qaxi(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_md_qaxi'" */
		infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_en_set(0x1);
		reg_write(INFRA_MDBUS_DCM_CTRL,
			(reg_read(INFRA_MDBUS_DCM_CTRL) &
			~INFRACFG_AO_MD_QAXI_REG0_MASK) |
			INFRACFG_AO_MD_QAXI_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_md_qaxi'" */
		infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_mdbus_dcm_dbc_rg_dbc_en_set(0x1);
		reg_write(INFRA_MDBUS_DCM_CTRL,
			(reg_read(INFRA_MDBUS_DCM_CTRL) &
			~INFRACFG_AO_MD_QAXI_REG0_MASK) |
			INFRACFG_AO_MD_QAXI_REG0_OFF);
	}
}

#define INFRACFG_AO_QAXI_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4))
#define INFRACFG_AO_QAXI_REG0_ON ((0x1 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4))
#define INFRACFG_AO_QAXI_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4))
#if 0
static unsigned int infracfg_ao_infra_qaxibus_dcm_rg_fsel_get(void)
{
	return (reg_read(INFRA_QAXIBUS_DCM_CTRL) >> 5) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_qaxibus_dcm_rg_sfsel_get(void)
{
	return (reg_read(INFRA_QAXIBUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_num_get(void)
{
	return (reg_read(INFRA_QAXIBUS_DCM_CTRL) >> 15) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_en_get(void)
{
	return (reg_read(INFRA_QAXIBUS_DCM_CTRL) >> 20) & 0x1;
}
#endif
static void infracfg_ao_infra_qaxibus_dcm_rg_fsel_set(unsigned int val)
{
	reg_write(INFRA_QAXIBUS_DCM_CTRL,
		(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
		~(0x1f << 5)) |
		(val & 0x1f) << 5);
}
static void infracfg_ao_infra_qaxibus_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(INFRA_QAXIBUS_DCM_CTRL,
		(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}
static void infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_num_set(unsigned int val)
{
	reg_write(INFRA_QAXIBUS_DCM_CTRL,
		(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
		~(0x1f << 15)) |
		(val & 0x1f) << 15);
}
static void infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_en_set(unsigned int val)
{
	reg_write(INFRA_QAXIBUS_DCM_CTRL,
		(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
		~(0x1 << 20)) |
		(val & 0x1) << 20);
}

bool dcm_infracfg_ao_qaxi_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(INFRA_QAXIBUS_DCM_CTRL) &
		INFRACFG_AO_QAXI_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_QAXI_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_qaxi(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_qaxi'" */
		infracfg_ao_infra_qaxibus_dcm_rg_fsel_set(0xf);
		infracfg_ao_infra_qaxibus_dcm_rg_sfsel_set(0xf);
		infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_en_set(0x1);
		reg_write(INFRA_QAXIBUS_DCM_CTRL,
			(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
			~INFRACFG_AO_QAXI_REG0_MASK) |
			INFRACFG_AO_QAXI_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_qaxi'" */
		infracfg_ao_infra_qaxibus_dcm_rg_fsel_set(0xf);
		infracfg_ao_infra_qaxibus_dcm_rg_sfsel_set(0xf);
		infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_num_set(0x10);
		infracfg_ao_infra_qaxibus_dcm_dbc_rg_dbc_en_set(0x1);
		reg_write(INFRA_QAXIBUS_DCM_CTRL,
			(reg_read(INFRA_QAXIBUS_DCM_CTRL) &
			~INFRACFG_AO_QAXI_REG0_MASK) |
			INFRACFG_AO_QAXI_REG0_OFF);
	}
}

#if 0
#define PWRAP_PMIC_WRAP_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13))
#define PWRAP_PMIC_WRAP_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13))
#define PWRAP_PMIC_WRAP_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13))

bool dcm_pwrap_pmic_wrap_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PMIC_WRAP_DCM_EN) &
		PWRAP_PMIC_WRAP_REG0_MASK) ==
		(unsigned int) PWRAP_PMIC_WRAP_REG0_ON);

	return ret;
}

void dcm_pwrap_pmic_wrap(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pwrap_pmic_wrap'" */
		reg_write(PMIC_WRAP_DCM_EN,
			(reg_read(PMIC_WRAP_DCM_EN) &
			~PWRAP_PMIC_WRAP_REG0_MASK) |
			PWRAP_PMIC_WRAP_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pwrap_pmic_wrap'" */
		reg_write(PMIC_WRAP_DCM_EN,
			(reg_read(PMIC_WRAP_DCM_EN) &
			~PWRAP_PMIC_WRAP_REG0_MASK) |
			PWRAP_PMIC_WRAP_REG0_OFF);
	}
}
#endif

#define MCUCFG_ADB400_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 11))
#define MCUCFG_ADB400_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 11))
#define MCUCFG_ADB400_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 11))

bool dcm_mcucfg_adb400_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CCI_ADB400_DCM_CONFIG) &
		MCUCFG_ADB400_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_ADB400_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_adb400_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_adb400_dcm'" */
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCUCFG_ADB400_DCM_REG0_MASK) |
			MCUCFG_ADB400_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_adb400_dcm'" */
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCUCFG_ADB400_DCM_REG0_MASK) |
			MCUCFG_ADB400_DCM_REG0_OFF);
	}
}

#define MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcucfg_bus_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(BUS_PLL_DIVIDER_CFG) &
		MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_bus_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_bus_arm_pll_divider_dcm'" */
		reg_write(BUS_PLL_DIVIDER_CFG,
			(reg_read(BUS_PLL_DIVIDER_CFG) &
			~MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_bus_arm_pll_divider_dcm'" */
		reg_write(BUS_PLL_DIVIDER_CFG,
			(reg_read(BUS_PLL_DIVIDER_CFG) &
			~MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCUCFG_BUS_SYNC_DCM_REG0_MASK ((0x1 << 0))
#define MCUCFG_BUS_SYNC_DCM_REG0_ON ((0x1 << 0))
#define MCUCFG_BUS_SYNC_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_bus_sync_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CONFIG) &
		MCUCFG_BUS_SYNC_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_BUS_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_bus_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_bus_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCUCFG_BUS_SYNC_DCM_REG0_MASK) |
			MCUCFG_BUS_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_bus_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCUCFG_BUS_SYNC_DCM_REG0_MASK) |
			MCUCFG_BUS_SYNC_DCM_REG0_OFF);
	}
}

#define MCUCFG_BUS_CLOCK_DCM_REG0_MASK ((0x1 << 8))
#define MCUCFG_BUS_CLOCK_DCM_REG0_ON ((0x1 << 8))
#define MCUCFG_BUS_CLOCK_DCM_REG0_OFF ((0x0 << 8))

bool dcm_mcucfg_bus_clock_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CCI_CLK_CTRL) &
		MCUCFG_BUS_CLOCK_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_BUS_CLOCK_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_bus_clock_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_bus_clock_dcm'" */
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCUCFG_BUS_CLOCK_DCM_REG0_MASK) |
			MCUCFG_BUS_CLOCK_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_bus_clock_dcm'" */
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCUCFG_BUS_CLOCK_DCM_REG0_MASK) |
			MCUCFG_BUS_CLOCK_DCM_REG0_OFF);
	}
}

#define MCUCFG_BUS_FABRIC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27))
#define MCUCFG_BUS_FABRIC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27))
#define MCUCFG_BUS_FABRIC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 18) | \
			(0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 21) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27))

bool dcm_mcucfg_bus_fabric_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(BUS_FABRIC_DCM_CTRL) &
		MCUCFG_BUS_FABRIC_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_BUS_FABRIC_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_bus_fabric_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_bus_fabric_dcm'" */
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCUCFG_BUS_FABRIC_DCM_REG0_MASK) |
			MCUCFG_BUS_FABRIC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_bus_fabric_dcm'" */
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCUCFG_BUS_FABRIC_DCM_REG0_MASK) |
			MCUCFG_BUS_FABRIC_DCM_REG0_OFF);
	}
}

#define MCUCFG_CNTVALUE_DCM_REG0_MASK ((0x1 << 2))
#define MCUCFG_CNTVALUE_DCM_REG0_ON ((0x1 << 2))
#define MCUCFG_CNTVALUE_DCM_REG0_OFF ((0x0 << 2))

bool dcm_mcucfg_cntvalue_dcm_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(MCUCFG_DBG_CONTROL) &
		MCUCFG_CNTVALUE_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_CNTVALUE_DCM_REG0_ON);

	return ret;
}

/* E1 is missing but this one is HW default on */
void dcm_mcucfg_cntvalue_dcm(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_cntvalue_dcm'" */
		reg_write(MCUCFG_DBG_CONTROL,
			(reg_read(MCUCFG_DBG_CONTROL) &
			~MCUCFG_CNTVALUE_DCM_REG0_MASK) |
			MCUCFG_CNTVALUE_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_cntvalue_dcm'" */
		reg_write(MCUCFG_DBG_CONTROL,
			(reg_read(MCUCFG_DBG_CONTROL) &
			~MCUCFG_CNTVALUE_DCM_REG0_MASK) |
			MCUCFG_CNTVALUE_DCM_REG0_OFF);
	}
}

#define MCUCFG_GIC_SYNC_DCM_REG0_MASK ((0x1 << 0))
#define MCUCFG_GIC_SYNC_DCM_REG0_ON ((0x1 << 0))
#define MCUCFG_GIC_SYNC_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_gic_sync_dcm_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(MCUSYS_GIC_SYNC_DCM) &
		MCUCFG_GIC_SYNC_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_GIC_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_gic_sync_dcm(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_gic_sync_dcm'" */
		reg_write(MCUSYS_GIC_SYNC_DCM,
			(reg_read(MCUSYS_GIC_SYNC_DCM) &
			~MCUCFG_GIC_SYNC_DCM_REG0_MASK) |
			MCUCFG_GIC_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_gic_sync_dcm'" */
		reg_write(MCUSYS_GIC_SYNC_DCM,
			(reg_read(MCUSYS_GIC_SYNC_DCM) &
			~MCUCFG_GIC_SYNC_DCM_REG0_MASK) |
			MCUCFG_GIC_SYNC_DCM_REG0_OFF);
	}
}

#define MCUCFG_L2_SHARED_DCM_REG0_MASK ((0x1 << 0))
#define MCUCFG_L2_SHARED_DCM_REG0_ON ((0x1 << 0))
#define MCUCFG_L2_SHARED_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_l2_shared_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(L2C_SRAM_CTRL) &
		MCUCFG_L2_SHARED_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_L2_SHARED_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_l2_shared_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_l2_shared_dcm'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCUCFG_L2_SHARED_DCM_REG0_MASK) |
			MCUCFG_L2_SHARED_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_l2_shared_dcm'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCUCFG_L2_SHARED_DCM_REG0_MASK) |
			MCUCFG_L2_SHARED_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_ADB_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2))
#define MCUCFG_MP0_ADB_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2))
#define MCUCFG_MP0_ADB_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2))

bool dcm_mcucfg_mp0_adb_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_SCAL_CCI_ADB400_DCM_CONFIG) &
		MCUCFG_MP0_ADB_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP0_ADB_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_adb_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_adb_dcm'" */
		reg_write(MP0_SCAL_CCI_ADB400_DCM_CONFIG,
			(reg_read(MP0_SCAL_CCI_ADB400_DCM_CONFIG) &
			~MCUCFG_MP0_ADB_DCM_REG0_MASK) |
			MCUCFG_MP0_ADB_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_adb_dcm'" */
		reg_write(MP0_SCAL_CCI_ADB400_DCM_CONFIG,
			(reg_read(MP0_SCAL_CCI_ADB400_DCM_CONFIG) &
			~MCUCFG_MP0_ADB_DCM_REG0_MASK) |
			MCUCFG_MP0_ADB_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcucfg_mp0_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_PLL_DIVIDER_CFG) &
		MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_arm_pll_divider_dcm'" */
		reg_write(MP0_PLL_DIVIDER_CFG,
			(reg_read(MP0_PLL_DIVIDER_CFG) &
			~MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_arm_pll_divider_dcm'" */
		reg_write(MP0_PLL_DIVIDER_CFG,
			(reg_read(MP0_PLL_DIVIDER_CFG) &
			~MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_BUS_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define MCUCFG_MP0_BUS_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define MCUCFG_MP0_BUS_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 2) | \
			(0x0 << 3))
#define MCUCFG_MP0_BUS_DCM_REG0_E2_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define MCUCFG_MP0_BUS_DCM_REG0_E2_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define MCUCFG_MP0_BUS_DCM_REG0_E2_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))

bool dcm_mcucfg_mp0_bus_dcm_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
		ret &= ((reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
			MCUCFG_MP0_BUS_DCM_REG0_E2_MASK) ==
			(unsigned int) MCUCFG_MP0_BUS_DCM_REG0_E2_ON);
	else
		ret &= ((reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
			MCUCFG_MP0_BUS_DCM_REG0_MASK) ==
			(unsigned int) MCUCFG_MP0_BUS_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_bus_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_bus_dcm'" */
		if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
			reg_write(MP0_SCAL_BUS_FABRIC_DCM_CTRL,
				(reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
				~MCUCFG_MP0_BUS_DCM_REG0_E2_MASK) |
				MCUCFG_MP0_BUS_DCM_REG0_E2_ON);
		else
			reg_write(MP0_SCAL_BUS_FABRIC_DCM_CTRL,
				(reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
				~MCUCFG_MP0_BUS_DCM_REG0_MASK) |
				MCUCFG_MP0_BUS_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_bus_dcm'" */
		if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
			reg_write(MP0_SCAL_BUS_FABRIC_DCM_CTRL,
				(reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
				~MCUCFG_MP0_BUS_DCM_REG0_E2_MASK) |
				MCUCFG_MP0_BUS_DCM_REG0_E2_OFF);
		else
			reg_write(MP0_SCAL_BUS_FABRIC_DCM_CTRL,
				(reg_read(MP0_SCAL_BUS_FABRIC_DCM_CTRL) &
				~MCUCFG_MP0_BUS_DCM_REG0_MASK) |
				MCUCFG_MP0_BUS_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MCUCFG_MP0_RGU_DCM_REG0_ON ((0x1 << 0))
#define MCUCFG_MP0_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_mp0_rgu_dcm_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(MCUCFG_MP0_RGU_DCM) &
		MCUCFG_MP0_RGU_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP0_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_rgu_dcm(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_rgu_dcm'" */
		reg_write(MCUCFG_MP0_RGU_DCM,
			(reg_read(MCUCFG_MP0_RGU_DCM) &
			~MCUCFG_MP0_RGU_DCM_REG0_MASK) |
			MCUCFG_MP0_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_rgu_dcm'" */
		reg_write(MCUCFG_MP0_RGU_DCM,
			(reg_read(MCUCFG_MP0_RGU_DCM) &
			~MCUCFG_MP0_RGU_DCM_REG0_MASK) |
			MCUCFG_MP0_RGU_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_STALL_DCM_REG0_MASK ((0x1 << 7))
#define MCUCFG_MP0_STALL_DCM_REG0_ON ((0x1 << 7))
#define MCUCFG_MP0_STALL_DCM_REG0_OFF ((0x0 << 7))

bool dcm_mcucfg_mp0_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG) &
		MCUCFG_MP0_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP0_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_stall_dcm'" */
		reg_write(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP0_STALL_DCM_REG0_MASK) |
			MCUCFG_MP0_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_stall_dcm'" */
		reg_write(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP0_STALL_DCM_REG0_MASK) |
			MCUCFG_MP0_STALL_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP0_SYNC_DCM_CFG_REG0_MASK ((0x1 << 0))
#define MCUCFG_MP0_SYNC_DCM_CFG_REG0_ON ((0x1 << 0))
#define MCUCFG_MP0_SYNC_DCM_CFG_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_mp0_sync_dcm_cfg_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_SCAL_SYNC_DCM_CONFIG) &
		MCUCFG_MP0_SYNC_DCM_CFG_REG0_MASK) ==
		(unsigned int) MCUCFG_MP0_SYNC_DCM_CFG_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp0_sync_dcm_cfg(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp0_sync_dcm_cfg'" */
		reg_write(MP0_SCAL_SYNC_DCM_CONFIG,
			(reg_read(MP0_SCAL_SYNC_DCM_CONFIG) &
			~MCUCFG_MP0_SYNC_DCM_CFG_REG0_MASK) |
			MCUCFG_MP0_SYNC_DCM_CFG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp0_sync_dcm_cfg'" */
		reg_write(MP0_SCAL_SYNC_DCM_CONFIG,
			(reg_read(MP0_SCAL_SYNC_DCM_CONFIG) &
			~MCUCFG_MP0_SYNC_DCM_CFG_REG0_MASK) |
			MCUCFG_MP0_SYNC_DCM_CFG_REG0_OFF);
	}
}

#define MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcucfg_mp1_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP1_PLL_DIVIDER_CFG) &
		MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp1_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp1_arm_pll_divider_dcm'" */
		reg_write(MP1_PLL_DIVIDER_CFG,
			(reg_read(MP1_PLL_DIVIDER_CFG) &
			~MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp1_arm_pll_divider_dcm'" */
		reg_write(MP1_PLL_DIVIDER_CFG,
			(reg_read(MP1_PLL_DIVIDER_CFG) &
			~MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP1_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MCUCFG_MP1_RGU_DCM_REG0_ON ((0x1 << 0))
#define MCUCFG_MP1_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_mp1_rgu_dcm_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(MCUCFG_MP1_RGU_DCM) &
		MCUCFG_MP1_RGU_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP1_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp1_rgu_dcm(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp1_rgu_dcm'" */
		reg_write(MCUCFG_MP1_RGU_DCM,
			(reg_read(MCUCFG_MP1_RGU_DCM) &
			~MCUCFG_MP1_RGU_DCM_REG0_MASK) |
			MCUCFG_MP1_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp1_rgu_dcm'" */
		reg_write(MCUCFG_MP1_RGU_DCM,
			(reg_read(MCUCFG_MP1_RGU_DCM) &
			~MCUCFG_MP1_RGU_DCM_REG0_MASK) |
			MCUCFG_MP1_RGU_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP1_STALL_DCM_REG0_MASK ((0x1 << 15))
#define MCUCFG_MP1_STALL_DCM_REG0_ON ((0x1 << 15))
#define MCUCFG_MP1_STALL_DCM_REG0_OFF ((0x0 << 15))

bool dcm_mcucfg_mp1_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		MCUCFG_MP1_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP1_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp1_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp1_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP1_STALL_DCM_REG0_MASK) |
			MCUCFG_MP1_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp1_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP1_STALL_DCM_REG0_MASK) |
			MCUCFG_MP1_STALL_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK ((0x1 << 16))
#define MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_ON ((0x1 << 16))
#define MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_OFF ((0x0 << 16))

bool dcm_mcucfg_mp1_sync_dcm_enable_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CONFIG) &
		MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) ==
		(unsigned int) MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp1_sync_dcm_enable(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp1_sync_dcm_enable'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) |
			MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp1_sync_dcm_enable'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) |
			MCUCFG_MP1_SYNC_DCM_ENABLE_REG0_OFF);
	}
}

#define MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcucfg_mp2_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP2_PLL_DIVIDER_CFG) &
		MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp2_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp2_arm_pll_divider_dcm'" */
		reg_write(MP2_PLL_DIVIDER_CFG,
			(reg_read(MP2_PLL_DIVIDER_CFG) &
			~MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp2_arm_pll_divider_dcm'" */
		reg_write(MP2_PLL_DIVIDER_CFG,
			(reg_read(MP2_PLL_DIVIDER_CFG) &
			~MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCUCFG_MP2_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCUCFG_MP_STALL_DCM_REG0_MASK ((0xf << 24))
#define MCUCFG_MP_STALL_DCM_REG0_ON ((0x5 << 24))
#define MCUCFG_MP_STALL_DCM_REG0_OFF ((0xf << 24))

bool dcm_mcucfg_mp_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		MCUCFG_MP_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MP_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mp_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mp_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP_STALL_DCM_REG0_MASK) |
			MCUCFG_MP_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mp_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCUCFG_MP_STALL_DCM_REG0_MASK) |
			MCUCFG_MP_STALL_DCM_REG0_OFF);
	}
}

#define MCUCFG_SYNC_DCM_CFG_REG0_MASK ((0x1 << 0))
#define MCUCFG_SYNC_DCM_CFG_REG0_ON ((0x1 << 0))
#define MCUCFG_SYNC_DCM_CFG_REG0_OFF ((0x0 << 0))

bool dcm_mcucfg_sync_dcm_cfg_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MCUCFG_SYNC_DCM) &
		MCUCFG_SYNC_DCM_CFG_REG0_MASK) ==
		(unsigned int) MCUCFG_SYNC_DCM_CFG_REG0_ON);

	return ret;
}

void dcm_mcucfg_sync_dcm_cfg(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_sync_dcm_cfg'" */
		reg_write(MCUCFG_SYNC_DCM,
			(reg_read(MCUCFG_SYNC_DCM) &
			~MCUCFG_SYNC_DCM_CFG_REG0_MASK) |
			MCUCFG_SYNC_DCM_CFG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_sync_dcm_cfg'" */
		reg_write(MCUCFG_SYNC_DCM,
			(reg_read(MCUCFG_SYNC_DCM) &
			~MCUCFG_SYNC_DCM_CFG_REG0_MASK) |
			MCUCFG_SYNC_DCM_CFG_REG0_OFF);
	}
}

#define MCUCFG_MCU_MISC_DCM_REG0_MASK ((0x1 << 1) | \
			(0x1 << 8))
#define MCUCFG_MCU_MISC_DCM_REG0_ON ((0x1 << 1) | \
			(0x1 << 8))
#define MCUCFG_MCU_MISC_DCM_REG0_OFF ((0x0 << 1) | \
			(0x0 << 8))

bool dcm_mcucfg_mcu_misc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MCU_MISC_DCM_CTRL) &
		MCUCFG_MCU_MISC_DCM_REG0_MASK) ==
		(unsigned int) MCUCFG_MCU_MISC_DCM_REG0_ON);

	return ret;
}

void dcm_mcucfg_mcu_misc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcucfg_mcu_misc_dcm'" */
		reg_write(MCU_MISC_DCM_CTRL,
			(reg_read(MCU_MISC_DCM_CTRL) &
			~MCUCFG_MCU_MISC_DCM_REG0_MASK) |
			MCUCFG_MCU_MISC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcucfg_mcu_misc_dcm'" */
		reg_write(MCU_MISC_DCM_CTRL,
			(reg_read(MCU_MISC_DCM_CTRL) &
			~MCUCFG_MCU_MISC_DCM_REG0_MASK) |
			MCUCFG_MCU_MISC_DCM_REG0_OFF);
	}
}

#define TOPCKGEN_CKSYS_DCM_EMI_REG0_MASK ((0x1 << 23) | \
			(0x7f << 24) | \
			(0x1 << 31))
#define TOPCKGEN_CKSYS_DCM_EMI_REG0_ON ((0x1 << 23) | \
			(0x8 << 24) | \
			(0x1 << 31))
#define TOPCKGEN_CKSYS_DCM_EMI_REG0_OFF ((0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 31))
#if 0
static unsigned int topckgen_emi_dcm_full_fsel_get(void)
{
	return (reg_read(TOPCKGEN_DCM_CFG) >> 16) & 0x1f;
}
#endif
unsigned int topckgen_emi_dcm_full_fsel_get(void)
{
	return ((reg_read(TOPCKGEN_DCM_CFG) & (0x1f << 16)) >> 16);
}

void topckgen_emi_dcm_full_fsel_set(unsigned int val)
{
	reg_write(TOPCKGEN_DCM_CFG,
		(reg_read(TOPCKGEN_DCM_CFG) &
		~(0x1f << 16)) |
		(val & 0x1f) << 16);
}

void topckgen_emi_dcm_dbc_cnt_set(unsigned int val)
{
	reg_write(TOPCKGEN_DCM_CFG,
		(reg_read(TOPCKGEN_DCM_CFG) &
		~(0x7f << 24)) |
		(val & 0x7f) << 24);
}

bool dcm_topckgen_cksys_dcm_emi_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(TOPCKGEN_DCM_CFG) &
		TOPCKGEN_CKSYS_DCM_EMI_REG0_MASK) ==
		(unsigned int) TOPCKGEN_CKSYS_DCM_EMI_REG0_ON);

	return ret;
}

void dcm_topckgen_cksys_dcm_emi(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'topckgen_cksys_dcm_emi'" */
		topckgen_emi_dcm_full_fsel_set(0x4); /* FIXME: workaround for MD EMI latency */
		reg_write(TOPCKGEN_DCM_CFG,
			(reg_read(TOPCKGEN_DCM_CFG) &
			~TOPCKGEN_CKSYS_DCM_EMI_REG0_MASK) |
			TOPCKGEN_CKSYS_DCM_EMI_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'topckgen_cksys_dcm_emi'" */
		topckgen_emi_dcm_full_fsel_set(0x0);
		reg_write(TOPCKGEN_DCM_CFG,
			(reg_read(TOPCKGEN_DCM_CFG) &
			~TOPCKGEN_CKSYS_DCM_EMI_REG0_MASK) |
			TOPCKGEN_CKSYS_DCM_EMI_REG0_OFF);
	}
}

#define EMI_EMI_DCM_REG_REG0_MASK ((0xff << 24))
#define EMI_EMI_DCM_REG_REG1_MASK ((0xff << 24))
#define EMI_EMI_DCM_REG_REG0_ON ((0x0 << 24))
#define EMI_EMI_DCM_REG_REG1_ON ((0x0 << 24))
#define EMI_EMI_DCM_REG_REG0_OFF ((0xff << 24))
#define EMI_EMI_DCM_REG_REG1_OFF ((0xff << 24))

bool dcm_emi_emi_dcm_reg_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(EMI_CONM) &
		EMI_EMI_DCM_REG_REG0_MASK) ==
		(unsigned int) EMI_EMI_DCM_REG_REG0_ON);
	ret &= ((reg_read(EMI_CONN) &
		EMI_EMI_DCM_REG_REG1_MASK) ==
		(unsigned int) EMI_EMI_DCM_REG_REG1_ON);

	return ret;
}

void dcm_emi_emi_dcm_reg(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'emi_emi_dcm_reg'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_EMI_DCM_REG_REG0_MASK) |
			EMI_EMI_DCM_REG_REG0_ON);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_EMI_DCM_REG_REG1_MASK) |
			EMI_EMI_DCM_REG_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'emi_emi_dcm_reg'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_EMI_DCM_REG_REG0_MASK) |
			EMI_EMI_DCM_REG_REG0_OFF);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_EMI_DCM_REG_REG1_MASK) |
			EMI_EMI_DCM_REG_REG1_OFF);
	}
}
#define CHN0_EMI_EMI_DCM_REG_REG0_MASK ((0xff << 24))
#define CHN0_EMI_EMI_DCM_REG_REG0_ON ((0x0 << 24))
#define CHN0_EMI_EMI_DCM_REG_REG0_OFF ((0xff << 24))

bool dcm_chn0_emi_emi_dcm_reg_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(CHN0_EMI_CHN_EMI_CONB) &
		CHN0_EMI_EMI_DCM_REG_REG0_MASK) ==
		(unsigned int) CHN0_EMI_EMI_DCM_REG_REG0_ON);

	return ret;
}

void dcm_chn0_emi_emi_dcm_reg(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'chn0_emi_emi_dcm_reg'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN0_EMI_EMI_DCM_REG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn0_emi_emi_dcm_reg'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN0_EMI_EMI_DCM_REG_REG0_OFF);
	}
}
#define CHN1_EMI_EMI_DCM_REG_REG0_MASK ((0xff << 24))
#define CHN1_EMI_EMI_DCM_REG_REG0_ON ((0x0 << 24))
#define CHN1_EMI_EMI_DCM_REG_REG0_OFF ((0xff << 24))

bool dcm_chn1_emi_emi_dcm_reg_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(CHN1_EMI_CHN_EMI_CONB) &
		CHN1_EMI_EMI_DCM_REG_REG0_MASK) ==
		(unsigned int) CHN1_EMI_EMI_DCM_REG_REG0_ON);

	return ret;
}

void dcm_chn1_emi_emi_dcm_reg(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'chn1_emi_emi_dcm_reg'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN1_EMI_EMI_DCM_REG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn1_emi_emi_dcm_reg'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN1_EMI_EMI_DCM_REG_REG0_OFF);
	}
}
#define CHN2_EMI_EMI_DCM_REG_REG0_MASK ((0xff << 24))
#define CHN2_EMI_EMI_DCM_REG_REG0_ON ((0x0 << 24))
#define CHN2_EMI_EMI_DCM_REG_REG0_OFF ((0xff << 24))

bool dcm_chn2_emi_emi_dcm_reg_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(CHN2_EMI_CHN_EMI_CONB) &
		CHN2_EMI_EMI_DCM_REG_REG0_MASK) ==
		(unsigned int) CHN2_EMI_EMI_DCM_REG_REG0_ON);

	return ret;
}

void dcm_chn2_emi_emi_dcm_reg(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'chn2_emi_emi_dcm_reg'" */
		reg_write(CHN2_EMI_CHN_EMI_CONB,
			(reg_read(CHN2_EMI_CHN_EMI_CONB) &
			~CHN2_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN2_EMI_EMI_DCM_REG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn2_emi_emi_dcm_reg'" */
		reg_write(CHN2_EMI_CHN_EMI_CONB,
			(reg_read(CHN2_EMI_CHN_EMI_CONB) &
			~CHN2_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN2_EMI_EMI_DCM_REG_REG0_OFF);
	}
}

#define CHN3_EMI_EMI_DCM_REG_REG0_MASK ((0xff << 24))
#define CHN3_EMI_EMI_DCM_REG_REG0_ON ((0x0 << 24))
#define CHN3_EMI_EMI_DCM_REG_REG0_OFF ((0xff << 24))

bool dcm_chn3_emi_emi_dcm_reg_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(CHN3_EMI_CHN_EMI_CONB) &
		CHN3_EMI_EMI_DCM_REG_REG0_MASK) ==
		(unsigned int) CHN3_EMI_EMI_DCM_REG_REG0_ON);

	return ret;
}

void dcm_chn3_emi_emi_dcm_reg(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'chn3_emi_emi_dcm_reg'" */
		reg_write(CHN3_EMI_CHN_EMI_CONB,
			(reg_read(CHN3_EMI_CHN_EMI_CONB) &
			~CHN3_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN3_EMI_EMI_DCM_REG_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn3_emi_emi_dcm_reg'" */
		reg_write(CHN3_EMI_CHN_EMI_CONB,
			(reg_read(CHN3_EMI_CHN_EMI_CONB) &
			~CHN3_EMI_EMI_DCM_REG_REG0_MASK) |
			CHN3_EMI_EMI_DCM_REG_REG0_OFF);
	}
}

#define LPDMA_LPDMA_REG0_MASK ((0x1 << 8))
#define LPDMA_LPDMA_REG0_ON ((0x0 << 8))
#define LPDMA_LPDMA_REG0_OFF ((0x1 << 8))

bool dcm_lpdma_lpdma_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(LPDMA_CONB) &
		LPDMA_LPDMA_REG0_MASK) ==
		(unsigned int) LPDMA_LPDMA_REG0_ON);

	return ret;
}

void dcm_lpdma_lpdma(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'lpdma_lpdma'" */
		reg_write(LPDMA_CONB,
			(reg_read(LPDMA_CONB) &
			~LPDMA_LPDMA_REG0_MASK) |
			LPDMA_LPDMA_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'lpdma_lpdma'" */
		reg_write(LPDMA_CONB,
			(reg_read(LPDMA_CONB) &
			~LPDMA_LPDMA_REG0_MASK) |
			LPDMA_LPDMA_REG0_OFF);
	}
}

#ifndef USE_DRAM_API_INSTEAD
#define DDRPHY0AO_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY0AO_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY0AO_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DDRPHY0AO_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7))
#define DDRPHY0AO_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY0AO_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY0AO_DDRPHY_REG1_TOG_MASK ((0x1 << 0))
#define DDRPHY0AO_DDRPHY_REG1_TOG1 ((0x0 << 0))
#define DDRPHY0AO_DDRPHY_REG1_TOG0 ((0x1 << 0))
#if 0
static unsigned int ddrphy0ao_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DDRPHY0AO_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void ddrphy0ao_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DDRPHY0AO_MISC_CG_CTRL2,
		(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_ddrphy0ao_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DDRPHY0AO_MISC_CG_CTRL0) &
		DDRPHY0AO_DDRPHY_REG0_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_REG0_ON);
	ret &= ((reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
		DDRPHY0AO_DDRPHY_REG1_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_REG1_ON);

	return ret;
}

void dcm_ddrphy0ao_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy0ao_ddrphy'" */
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY0AO_DDRPHY_REG1_TOG0);
		ddrphy0ao_rg_mem_dcm_idle_fsel_set(0x1f);
		reg_write(DDRPHY0AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL0) &
			~DDRPHY0AO_DDRPHY_REG0_MASK) |
			DDRPHY0AO_DDRPHY_REG0_ON);
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_MASK) |
			DDRPHY0AO_DDRPHY_REG1_ON);
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY0AO_DDRPHY_REG1_TOG1);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy0ao_ddrphy'" */
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY0AO_DDRPHY_REG1_TOG0);
		ddrphy0ao_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DDRPHY0AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL0) &
			~DDRPHY0AO_DDRPHY_REG0_MASK) |
			DDRPHY0AO_DDRPHY_REG0_OFF);
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_MASK) |
			DDRPHY0AO_DDRPHY_REG1_OFF);
		reg_write(DDRPHY0AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL2) &
			~DDRPHY0AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY0AO_DDRPHY_REG1_TOG1);
	}
}

#define DDRPHY0AO_DDRPHY_E2_REG0_MASK ((0xffffffff << 0))
#define DDRPHY0AO_DDRPHY_E2_REG1_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY0AO_DDRPHY_E2_REG2_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG3_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG4_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG5_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG6_MASK ((0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG7_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG8_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG9_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG10_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG11_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG12_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG13_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG14_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG15_MASK ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG16_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG17_MASK ((0xffffffff << 0))
#define DDRPHY0AO_DDRPHY_E2_REG0_ON ((0x470000 << 0))
#define DDRPHY0AO_DDRPHY_E2_REG1_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DDRPHY0AO_DDRPHY_E2_REG2_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG3_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG4_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG5_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG6_ON ((0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG7_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG8_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG9_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG10_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG11_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG12_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG13_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG14_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG15_ON ((0x2 << 20))
#define DDRPHY0AO_DDRPHY_E2_REG16_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG17_ON ((0x2 << 0))
#define DDRPHY0AO_DDRPHY_E2_REG0_OFF ((0x0 << 0))
#define DDRPHY0AO_DDRPHY_E2_REG1_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY0AO_DDRPHY_E2_REG2_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG3_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG4_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG5_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG6_OFF ((0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG7_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG8_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG9_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG10_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG11_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG12_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG13_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG14_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG15_OFF ((0xfff << 20))
#define DDRPHY0AO_DDRPHY_E2_REG16_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY0AO_DDRPHY_E2_REG17_OFF ((0xfff << 0))

bool dcm_ddrphy0ao_ddrphy_e2_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(DDRPHY0AO_MISC_CG_CTRL5) &
		DDRPHY0AO_DDRPHY_E2_REG0_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG0_ON);
	ret &= ((reg_read(DDRPHY0AO_MISC_CTRL3) &
		DDRPHY0AO_DDRPHY_E2_REG1_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG1_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU1_B0_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG2_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG2_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0XC20) &
		DDRPHY0AO_DDRPHY_E2_REG3_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG3_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU1_B1_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG4_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG4_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0XCA0) &
		DDRPHY0AO_DDRPHY_E2_REG5_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG5_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU2_B0_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG6_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG6_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X1120) &
		DDRPHY0AO_DDRPHY_E2_REG7_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG7_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU2_B1_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG8_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG8_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X11A0) &
		DDRPHY0AO_DDRPHY_E2_REG9_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG9_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU3_B0_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG10_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG10_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X1620) &
		DDRPHY0AO_DDRPHY_E2_REG11_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG11_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU3_B1_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG12_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG12_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X16A0) &
		DDRPHY0AO_DDRPHY_E2_REG13_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG13_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU4_B0_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG14_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG14_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X1B20) &
		DDRPHY0AO_DDRPHY_E2_REG15_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG15_ON);
	ret &= ((reg_read(DDRPHY0AO_SHU4_B1_DQ0) &
		DDRPHY0AO_DDRPHY_E2_REG16_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG16_ON);
	ret &= ((reg_read(DDRPHY0AO_RFU_0X1BA0) &
		DDRPHY0AO_DDRPHY_E2_REG17_MASK) ==
		(unsigned int) DDRPHY0AO_DDRPHY_E2_REG17_ON);

	return ret;
}

void dcm_ddrphy0ao_ddrphy_e2(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy0ao_ddrphy_e2'" */
		reg_write(DDRPHY0AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL5) &
			~DDRPHY0AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG0_ON);
		reg_write(DDRPHY0AO_MISC_CTRL3,
			(reg_read(DDRPHY0AO_MISC_CTRL3) &
			~DDRPHY0AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG1_ON);
		reg_write(DDRPHY0AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU1_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG2_ON);
		reg_write(DDRPHY0AO_RFU_0XC20,
			(reg_read(DDRPHY0AO_RFU_0XC20) &
			~DDRPHY0AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG3_ON);
		reg_write(DDRPHY0AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU1_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG4_ON);
		reg_write(DDRPHY0AO_RFU_0XCA0,
			(reg_read(DDRPHY0AO_RFU_0XCA0) &
			~DDRPHY0AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG5_ON);
		reg_write(DDRPHY0AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU2_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG6_ON);
		reg_write(DDRPHY0AO_RFU_0X1120,
			(reg_read(DDRPHY0AO_RFU_0X1120) &
			~DDRPHY0AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG7_ON);
		reg_write(DDRPHY0AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU2_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG8_ON);
		reg_write(DDRPHY0AO_RFU_0X11A0,
			(reg_read(DDRPHY0AO_RFU_0X11A0) &
			~DDRPHY0AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG9_ON);
		reg_write(DDRPHY0AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU3_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG10_ON);
		reg_write(DDRPHY0AO_RFU_0X1620,
			(reg_read(DDRPHY0AO_RFU_0X1620) &
			~DDRPHY0AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG11_ON);
		reg_write(DDRPHY0AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU3_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG12_ON);
		reg_write(DDRPHY0AO_RFU_0X16A0,
			(reg_read(DDRPHY0AO_RFU_0X16A0) &
			~DDRPHY0AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG13_ON);
		reg_write(DDRPHY0AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU4_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG14_ON);
		reg_write(DDRPHY0AO_RFU_0X1B20,
			(reg_read(DDRPHY0AO_RFU_0X1B20) &
			~DDRPHY0AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG15_ON);
		reg_write(DDRPHY0AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU4_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG16_ON);
		reg_write(DDRPHY0AO_RFU_0X1BA0,
			(reg_read(DDRPHY0AO_RFU_0X1BA0) &
			~DDRPHY0AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG17_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy0ao_ddrphy_e2'" */
		reg_write(DDRPHY0AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY0AO_MISC_CG_CTRL5) &
			~DDRPHY0AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG0_OFF);
		reg_write(DDRPHY0AO_MISC_CTRL3,
			(reg_read(DDRPHY0AO_MISC_CTRL3) &
			~DDRPHY0AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG1_OFF);
		reg_write(DDRPHY0AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU1_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG2_OFF);
		reg_write(DDRPHY0AO_RFU_0XC20,
			(reg_read(DDRPHY0AO_RFU_0XC20) &
			~DDRPHY0AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG3_OFF);
		reg_write(DDRPHY0AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU1_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG4_OFF);
		reg_write(DDRPHY0AO_RFU_0XCA0,
			(reg_read(DDRPHY0AO_RFU_0XCA0) &
			~DDRPHY0AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG5_OFF);
		reg_write(DDRPHY0AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU2_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG6_OFF);
		reg_write(DDRPHY0AO_RFU_0X1120,
			(reg_read(DDRPHY0AO_RFU_0X1120) &
			~DDRPHY0AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG7_OFF);
		reg_write(DDRPHY0AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU2_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG8_OFF);
		reg_write(DDRPHY0AO_RFU_0X11A0,
			(reg_read(DDRPHY0AO_RFU_0X11A0) &
			~DDRPHY0AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG9_OFF);
		reg_write(DDRPHY0AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU3_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG10_OFF);
		reg_write(DDRPHY0AO_RFU_0X1620,
			(reg_read(DDRPHY0AO_RFU_0X1620) &
			~DDRPHY0AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG11_OFF);
		reg_write(DDRPHY0AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU3_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG12_OFF);
		reg_write(DDRPHY0AO_RFU_0X16A0,
			(reg_read(DDRPHY0AO_RFU_0X16A0) &
			~DDRPHY0AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG13_OFF);
		reg_write(DDRPHY0AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY0AO_SHU4_B0_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG14_OFF);
		reg_write(DDRPHY0AO_RFU_0X1B20,
			(reg_read(DDRPHY0AO_RFU_0X1B20) &
			~DDRPHY0AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG15_OFF);
		reg_write(DDRPHY0AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY0AO_SHU4_B1_DQ0) &
			~DDRPHY0AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG16_OFF);
		reg_write(DDRPHY0AO_RFU_0X1BA0,
			(reg_read(DDRPHY0AO_RFU_0X1BA0) &
			~DDRPHY0AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY0AO_DDRPHY_E2_REG17_OFF);
	}
}

#define DRAMC0_AO_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC0_AO_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC0_AO_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC0_AO_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC0_AO_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC0_AO_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc0_ao_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC0_AO_DRAMC_PD_CTRL) &
		DRAMC0_AO_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC0_AO_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC0_AO_CLKAR) &
		DRAMC0_AO_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC0_AO_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc0_ao_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc0_ao_dramc_dcm'" */
		reg_write(DRAMC0_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC0_AO_DRAMC_PD_CTRL) &
			~DRAMC0_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC0_AO_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC0_AO_CLKAR,
			(reg_read(DRAMC0_AO_CLKAR) &
			~DRAMC0_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC0_AO_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc0_ao_dramc_dcm'" */
		reg_write(DRAMC0_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC0_AO_DRAMC_PD_CTRL) &
			~DRAMC0_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC0_AO_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC0_AO_CLKAR,
			(reg_read(DRAMC0_AO_CLKAR) &
			~DRAMC0_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC0_AO_DRAMC_DCM_REG1_OFF);
	}
}

#define DDRPHY1AO_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY1AO_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY1AO_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DDRPHY1AO_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7))
#define DDRPHY1AO_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY1AO_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY1AO_DDRPHY_REG1_TOG_MASK ((0x1 << 0))
#define DDRPHY1AO_DDRPHY_REG1_TOG1 ((0x0 << 0))
#define DDRPHY1AO_DDRPHY_REG1_TOG0 ((0x1 << 0))
#if 0
static unsigned int ddrphy1ao_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DDRPHY1AO_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void ddrphy1ao_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DDRPHY1AO_MISC_CG_CTRL2,
		(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_ddrphy1ao_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DDRPHY1AO_MISC_CG_CTRL0) &
		DDRPHY1AO_DDRPHY_REG0_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_REG0_ON);
	ret &= ((reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
		DDRPHY1AO_DDRPHY_REG1_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_REG1_ON);

	return ret;
}

void dcm_ddrphy1ao_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy1ao_ddrphy'" */
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY1AO_DDRPHY_REG1_TOG0);
		ddrphy1ao_rg_mem_dcm_idle_fsel_set(0x1f);
		reg_write(DDRPHY1AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL0) &
			~DDRPHY1AO_DDRPHY_REG0_MASK) |
			DDRPHY1AO_DDRPHY_REG0_ON);
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_MASK) |
			DDRPHY1AO_DDRPHY_REG1_ON);
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY1AO_DDRPHY_REG1_TOG1);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy1ao_ddrphy'" */
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY1AO_DDRPHY_REG1_TOG0);
		ddrphy1ao_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DDRPHY1AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL0) &
			~DDRPHY1AO_DDRPHY_REG0_MASK) |
			DDRPHY1AO_DDRPHY_REG0_OFF);
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_MASK) |
			DDRPHY1AO_DDRPHY_REG1_OFF);
		reg_write(DDRPHY1AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL2) &
			~DDRPHY1AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY1AO_DDRPHY_REG1_TOG1);
	}
}

#define DDRPHY1AO_DDRPHY_E2_REG0_MASK ((0xffffffff << 0))
#define DDRPHY1AO_DDRPHY_E2_REG1_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY1AO_DDRPHY_E2_REG2_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG3_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG4_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG5_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG6_MASK ((0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG7_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG8_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG9_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG10_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG11_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG12_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG13_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG14_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG15_MASK ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG16_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG17_MASK ((0xffffffff << 0))
#define DDRPHY1AO_DDRPHY_E2_REG0_ON ((0x470000 << 0))
#define DDRPHY1AO_DDRPHY_E2_REG1_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DDRPHY1AO_DDRPHY_E2_REG2_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG3_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG4_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG5_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG6_ON ((0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG7_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG8_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG9_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG10_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG11_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG12_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG13_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG14_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG15_ON ((0x2 << 20))
#define DDRPHY1AO_DDRPHY_E2_REG16_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG17_ON ((0x2 << 0))
#define DDRPHY1AO_DDRPHY_E2_REG0_OFF ((0x0 << 0))
#define DDRPHY1AO_DDRPHY_E2_REG1_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY1AO_DDRPHY_E2_REG2_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG3_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG4_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG5_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG6_OFF ((0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG7_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG8_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG9_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG10_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG11_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG12_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG13_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG14_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG15_OFF ((0xfff << 20))
#define DDRPHY1AO_DDRPHY_E2_REG16_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY1AO_DDRPHY_E2_REG17_OFF ((0xfff << 0))

bool dcm_ddrphy1ao_ddrphy_e2_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(DDRPHY1AO_MISC_CG_CTRL5) &
		DDRPHY1AO_DDRPHY_E2_REG0_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG0_ON);
	ret &= ((reg_read(DDRPHY1AO_MISC_CTRL3) &
		DDRPHY1AO_DDRPHY_E2_REG1_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG1_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU1_B0_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG2_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG2_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0XC20) &
		DDRPHY1AO_DDRPHY_E2_REG3_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG3_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU1_B1_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG4_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG4_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0XCA0) &
		DDRPHY1AO_DDRPHY_E2_REG5_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG5_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU2_B0_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG6_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG6_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X1120) &
		DDRPHY1AO_DDRPHY_E2_REG7_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG7_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU2_B1_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG8_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG8_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X11A0) &
		DDRPHY1AO_DDRPHY_E2_REG9_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG9_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU3_B0_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG10_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG10_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X1620) &
		DDRPHY1AO_DDRPHY_E2_REG11_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG11_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU3_B1_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG12_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG12_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X16A0) &
		DDRPHY1AO_DDRPHY_E2_REG13_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG13_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU4_B0_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG14_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG14_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X1B20) &
		DDRPHY1AO_DDRPHY_E2_REG15_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG15_ON);
	ret &= ((reg_read(DDRPHY1AO_SHU4_B1_DQ0) &
		DDRPHY1AO_DDRPHY_E2_REG16_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG16_ON);
	ret &= ((reg_read(DDRPHY1AO_RFU_0X1BA0) &
		DDRPHY1AO_DDRPHY_E2_REG17_MASK) ==
		(unsigned int) DDRPHY1AO_DDRPHY_E2_REG17_ON);

	return ret;
}

void dcm_ddrphy1ao_ddrphy_e2(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy1ao_ddrphy_e2'" */
		reg_write(DDRPHY1AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL5) &
			~DDRPHY1AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG0_ON);
		reg_write(DDRPHY1AO_MISC_CTRL3,
			(reg_read(DDRPHY1AO_MISC_CTRL3) &
			~DDRPHY1AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG1_ON);
		reg_write(DDRPHY1AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU1_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG2_ON);
		reg_write(DDRPHY1AO_RFU_0XC20,
			(reg_read(DDRPHY1AO_RFU_0XC20) &
			~DDRPHY1AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG3_ON);
		reg_write(DDRPHY1AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU1_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG4_ON);
		reg_write(DDRPHY1AO_RFU_0XCA0,
			(reg_read(DDRPHY1AO_RFU_0XCA0) &
			~DDRPHY1AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG5_ON);
		reg_write(DDRPHY1AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU2_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG6_ON);
		reg_write(DDRPHY1AO_RFU_0X1120,
			(reg_read(DDRPHY1AO_RFU_0X1120) &
			~DDRPHY1AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG7_ON);
		reg_write(DDRPHY1AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU2_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG8_ON);
		reg_write(DDRPHY1AO_RFU_0X11A0,
			(reg_read(DDRPHY1AO_RFU_0X11A0) &
			~DDRPHY1AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG9_ON);
		reg_write(DDRPHY1AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU3_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG10_ON);
		reg_write(DDRPHY1AO_RFU_0X1620,
			(reg_read(DDRPHY1AO_RFU_0X1620) &
			~DDRPHY1AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG11_ON);
		reg_write(DDRPHY1AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU3_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG12_ON);
		reg_write(DDRPHY1AO_RFU_0X16A0,
			(reg_read(DDRPHY1AO_RFU_0X16A0) &
			~DDRPHY1AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG13_ON);
		reg_write(DDRPHY1AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU4_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG14_ON);
		reg_write(DDRPHY1AO_RFU_0X1B20,
			(reg_read(DDRPHY1AO_RFU_0X1B20) &
			~DDRPHY1AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG15_ON);
		reg_write(DDRPHY1AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU4_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG16_ON);
		reg_write(DDRPHY1AO_RFU_0X1BA0,
			(reg_read(DDRPHY1AO_RFU_0X1BA0) &
			~DDRPHY1AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG17_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy1ao_ddrphy_e2'" */
		reg_write(DDRPHY1AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY1AO_MISC_CG_CTRL5) &
			~DDRPHY1AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG0_OFF);
		reg_write(DDRPHY1AO_MISC_CTRL3,
			(reg_read(DDRPHY1AO_MISC_CTRL3) &
			~DDRPHY1AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG1_OFF);
		reg_write(DDRPHY1AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU1_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG2_OFF);
		reg_write(DDRPHY1AO_RFU_0XC20,
			(reg_read(DDRPHY1AO_RFU_0XC20) &
			~DDRPHY1AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG3_OFF);
		reg_write(DDRPHY1AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU1_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG4_OFF);
		reg_write(DDRPHY1AO_RFU_0XCA0,
			(reg_read(DDRPHY1AO_RFU_0XCA0) &
			~DDRPHY1AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG5_OFF);
		reg_write(DDRPHY1AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU2_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG6_OFF);
		reg_write(DDRPHY1AO_RFU_0X1120,
			(reg_read(DDRPHY1AO_RFU_0X1120) &
			~DDRPHY1AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG7_OFF);
		reg_write(DDRPHY1AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU2_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG8_OFF);
		reg_write(DDRPHY1AO_RFU_0X11A0,
			(reg_read(DDRPHY1AO_RFU_0X11A0) &
			~DDRPHY1AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG9_OFF);
		reg_write(DDRPHY1AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU3_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG10_OFF);
		reg_write(DDRPHY1AO_RFU_0X1620,
			(reg_read(DDRPHY1AO_RFU_0X1620) &
			~DDRPHY1AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG11_OFF);
		reg_write(DDRPHY1AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU3_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG12_OFF);
		reg_write(DDRPHY1AO_RFU_0X16A0,
			(reg_read(DDRPHY1AO_RFU_0X16A0) &
			~DDRPHY1AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG13_OFF);
		reg_write(DDRPHY1AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY1AO_SHU4_B0_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG14_OFF);
		reg_write(DDRPHY1AO_RFU_0X1B20,
			(reg_read(DDRPHY1AO_RFU_0X1B20) &
			~DDRPHY1AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG15_OFF);
		reg_write(DDRPHY1AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY1AO_SHU4_B1_DQ0) &
			~DDRPHY1AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG16_OFF);
		reg_write(DDRPHY1AO_RFU_0X1BA0,
			(reg_read(DDRPHY1AO_RFU_0X1BA0) &
			~DDRPHY1AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY1AO_DDRPHY_E2_REG17_OFF);
	}
}

#define DRAMC1_AO_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC1_AO_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC1_AO_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC1_AO_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC1_AO_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC1_AO_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc1_ao_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC1_AO_DRAMC_PD_CTRL) &
		DRAMC1_AO_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC1_AO_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC1_AO_CLKAR) &
		DRAMC1_AO_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC1_AO_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc1_ao_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc1_ao_dramc_dcm'" */
		reg_write(DRAMC1_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC1_AO_DRAMC_PD_CTRL) &
			~DRAMC1_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC1_AO_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC1_AO_CLKAR,
			(reg_read(DRAMC1_AO_CLKAR) &
			~DRAMC1_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC1_AO_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc1_ao_dramc_dcm'" */
		reg_write(DRAMC1_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC1_AO_DRAMC_PD_CTRL) &
			~DRAMC1_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC1_AO_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC1_AO_CLKAR,
			(reg_read(DRAMC1_AO_CLKAR) &
			~DRAMC1_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC1_AO_DRAMC_DCM_REG1_OFF);
	}
}

#define DDRPHY2AO_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY2AO_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY2AO_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DDRPHY2AO_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7))
#define DDRPHY2AO_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY2AO_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY2AO_DDRPHY_REG1_TOG_MASK ((0x1 << 0))
#define DDRPHY2AO_DDRPHY_REG1_TOG1 ((0x0 << 0))
#define DDRPHY2AO_DDRPHY_REG1_TOG0 ((0x1 << 0))
#if 0
static unsigned int ddrphy2ao_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DDRPHY2AO_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void ddrphy2ao_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DDRPHY2AO_MISC_CG_CTRL2,
		(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_ddrphy2ao_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DDRPHY2AO_MISC_CG_CTRL0) &
		DDRPHY2AO_DDRPHY_REG0_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_REG0_ON);
	ret &= ((reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
		DDRPHY2AO_DDRPHY_REG1_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_REG1_ON);

	return ret;
}

void dcm_ddrphy2ao_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy2ao_ddrphy'" */
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY2AO_DDRPHY_REG1_TOG0);
		ddrphy2ao_rg_mem_dcm_idle_fsel_set(0x1f);
		reg_write(DDRPHY2AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL0) &
			~DDRPHY2AO_DDRPHY_REG0_MASK) |
			DDRPHY2AO_DDRPHY_REG0_ON);
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_MASK) |
			DDRPHY2AO_DDRPHY_REG1_ON);
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY2AO_DDRPHY_REG1_TOG1);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy2ao_ddrphy'" */
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY2AO_DDRPHY_REG1_TOG0);
		ddrphy2ao_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DDRPHY2AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL0) &
			~DDRPHY2AO_DDRPHY_REG0_MASK) |
			DDRPHY2AO_DDRPHY_REG0_OFF);
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_MASK) |
			DDRPHY2AO_DDRPHY_REG1_OFF);
		reg_write(DDRPHY2AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL2) &
			~DDRPHY2AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY2AO_DDRPHY_REG1_TOG1);
	}
}

#define DDRPHY2AO_DDRPHY_E2_REG0_MASK ((0xffffffff << 0))
#define DDRPHY2AO_DDRPHY_E2_REG1_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY2AO_DDRPHY_E2_REG2_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG3_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG4_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG5_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG6_MASK ((0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG7_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG8_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG9_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG10_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG11_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG12_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG13_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG14_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG15_MASK ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG16_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG17_MASK ((0xffffffff << 0))
#define DDRPHY2AO_DDRPHY_E2_REG0_ON ((0x470000 << 0))
#define DDRPHY2AO_DDRPHY_E2_REG1_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DDRPHY2AO_DDRPHY_E2_REG2_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG3_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG4_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG5_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG6_ON ((0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG7_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG8_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG9_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG10_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG11_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG12_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG13_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG14_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG15_ON ((0x2 << 20))
#define DDRPHY2AO_DDRPHY_E2_REG16_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG17_ON ((0x2 << 0))
#define DDRPHY2AO_DDRPHY_E2_REG0_OFF ((0x0 << 0))
#define DDRPHY2AO_DDRPHY_E2_REG1_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY2AO_DDRPHY_E2_REG2_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG3_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG4_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG5_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG6_OFF ((0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG7_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG8_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG9_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG10_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG11_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG12_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG13_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG14_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG15_OFF ((0xfff << 20))
#define DDRPHY2AO_DDRPHY_E2_REG16_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY2AO_DDRPHY_E2_REG17_OFF ((0xfff << 0))

bool dcm_ddrphy2ao_ddrphy_e2_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(DDRPHY2AO_MISC_CG_CTRL5) &
		DDRPHY2AO_DDRPHY_E2_REG0_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG0_ON);
	ret &= ((reg_read(DDRPHY2AO_MISC_CTRL3) &
		DDRPHY2AO_DDRPHY_E2_REG1_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG1_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU1_B0_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG2_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG2_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0XC20) &
		DDRPHY2AO_DDRPHY_E2_REG3_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG3_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU1_B1_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG4_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG4_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0XCA0) &
		DDRPHY2AO_DDRPHY_E2_REG5_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG5_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU2_B0_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG6_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG6_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X1120) &
		DDRPHY2AO_DDRPHY_E2_REG7_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG7_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU2_B1_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG8_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG8_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X11A0) &
		DDRPHY2AO_DDRPHY_E2_REG9_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG9_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU3_B0_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG10_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG10_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X1620) &
		DDRPHY2AO_DDRPHY_E2_REG11_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG11_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU3_B1_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG12_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG12_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X16A0) &
		DDRPHY2AO_DDRPHY_E2_REG13_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG13_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU4_B0_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG14_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG14_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X1B20) &
		DDRPHY2AO_DDRPHY_E2_REG15_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG15_ON);
	ret &= ((reg_read(DDRPHY2AO_SHU4_B1_DQ0) &
		DDRPHY2AO_DDRPHY_E2_REG16_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG16_ON);
	ret &= ((reg_read(DDRPHY2AO_RFU_0X1BA0) &
		DDRPHY2AO_DDRPHY_E2_REG17_MASK) ==
		(unsigned int) DDRPHY2AO_DDRPHY_E2_REG17_ON);

	return ret;
}

void dcm_ddrphy2ao_ddrphy_e2(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy2ao_ddrphy_e2'" */
		reg_write(DDRPHY2AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL5) &
			~DDRPHY2AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG0_ON);
		reg_write(DDRPHY2AO_MISC_CTRL3,
			(reg_read(DDRPHY2AO_MISC_CTRL3) &
			~DDRPHY2AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG1_ON);
		reg_write(DDRPHY2AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU1_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG2_ON);
		reg_write(DDRPHY2AO_RFU_0XC20,
			(reg_read(DDRPHY2AO_RFU_0XC20) &
			~DDRPHY2AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG3_ON);
		reg_write(DDRPHY2AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU1_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG4_ON);
		reg_write(DDRPHY2AO_RFU_0XCA0,
			(reg_read(DDRPHY2AO_RFU_0XCA0) &
			~DDRPHY2AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG5_ON);
		reg_write(DDRPHY2AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU2_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG6_ON);
		reg_write(DDRPHY2AO_RFU_0X1120,
			(reg_read(DDRPHY2AO_RFU_0X1120) &
			~DDRPHY2AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG7_ON);
		reg_write(DDRPHY2AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU2_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG8_ON);
		reg_write(DDRPHY2AO_RFU_0X11A0,
			(reg_read(DDRPHY2AO_RFU_0X11A0) &
			~DDRPHY2AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG9_ON);
		reg_write(DDRPHY2AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU3_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG10_ON);
		reg_write(DDRPHY2AO_RFU_0X1620,
			(reg_read(DDRPHY2AO_RFU_0X1620) &
			~DDRPHY2AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG11_ON);
		reg_write(DDRPHY2AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU3_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG12_ON);
		reg_write(DDRPHY2AO_RFU_0X16A0,
			(reg_read(DDRPHY2AO_RFU_0X16A0) &
			~DDRPHY2AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG13_ON);
		reg_write(DDRPHY2AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU4_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG14_ON);
		reg_write(DDRPHY2AO_RFU_0X1B20,
			(reg_read(DDRPHY2AO_RFU_0X1B20) &
			~DDRPHY2AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG15_ON);
		reg_write(DDRPHY2AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU4_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG16_ON);
		reg_write(DDRPHY2AO_RFU_0X1BA0,
			(reg_read(DDRPHY2AO_RFU_0X1BA0) &
			~DDRPHY2AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG17_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy2ao_ddrphy_e2'" */
		reg_write(DDRPHY2AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY2AO_MISC_CG_CTRL5) &
			~DDRPHY2AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG0_OFF);
		reg_write(DDRPHY2AO_MISC_CTRL3,
			(reg_read(DDRPHY2AO_MISC_CTRL3) &
			~DDRPHY2AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG1_OFF);
		reg_write(DDRPHY2AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU1_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG2_OFF);
		reg_write(DDRPHY2AO_RFU_0XC20,
			(reg_read(DDRPHY2AO_RFU_0XC20) &
			~DDRPHY2AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG3_OFF);
		reg_write(DDRPHY2AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU1_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG4_OFF);
		reg_write(DDRPHY2AO_RFU_0XCA0,
			(reg_read(DDRPHY2AO_RFU_0XCA0) &
			~DDRPHY2AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG5_OFF);
		reg_write(DDRPHY2AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU2_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG6_OFF);
		reg_write(DDRPHY2AO_RFU_0X1120,
			(reg_read(DDRPHY2AO_RFU_0X1120) &
			~DDRPHY2AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG7_OFF);
		reg_write(DDRPHY2AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU2_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG8_OFF);
		reg_write(DDRPHY2AO_RFU_0X11A0,
			(reg_read(DDRPHY2AO_RFU_0X11A0) &
			~DDRPHY2AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG9_OFF);
		reg_write(DDRPHY2AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU3_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG10_OFF);
		reg_write(DDRPHY2AO_RFU_0X1620,
			(reg_read(DDRPHY2AO_RFU_0X1620) &
			~DDRPHY2AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG11_OFF);
		reg_write(DDRPHY2AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU3_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG12_OFF);
		reg_write(DDRPHY2AO_RFU_0X16A0,
			(reg_read(DDRPHY2AO_RFU_0X16A0) &
			~DDRPHY2AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG13_OFF);
		reg_write(DDRPHY2AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY2AO_SHU4_B0_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG14_OFF);
		reg_write(DDRPHY2AO_RFU_0X1B20,
			(reg_read(DDRPHY2AO_RFU_0X1B20) &
			~DDRPHY2AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG15_OFF);
		reg_write(DDRPHY2AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY2AO_SHU4_B1_DQ0) &
			~DDRPHY2AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG16_OFF);
		reg_write(DDRPHY2AO_RFU_0X1BA0,
			(reg_read(DDRPHY2AO_RFU_0X1BA0) &
			~DDRPHY2AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY2AO_DDRPHY_E2_REG17_OFF);
	}
}

#define DRAMC2_AO_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC2_AO_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC2_AO_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC2_AO_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC2_AO_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC2_AO_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc2_ao_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC2_AO_DRAMC_PD_CTRL) &
		DRAMC2_AO_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC2_AO_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC2_AO_CLKAR) &
		DRAMC2_AO_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC2_AO_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc2_ao_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc2_ao_dramc_dcm'" */
		reg_write(DRAMC2_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC2_AO_DRAMC_PD_CTRL) &
			~DRAMC2_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC2_AO_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC2_AO_CLKAR,
			(reg_read(DRAMC2_AO_CLKAR) &
			~DRAMC2_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC2_AO_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc2_ao_dramc_dcm'" */
		reg_write(DRAMC2_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC2_AO_DRAMC_PD_CTRL) &
			~DRAMC2_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC2_AO_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC2_AO_CLKAR,
			(reg_read(DRAMC2_AO_CLKAR) &
			~DRAMC2_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC2_AO_DRAMC_DCM_REG1_OFF);
	}
}

#define DDRPHY3AO_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY3AO_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY3AO_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DDRPHY3AO_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7))
#define DDRPHY3AO_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DDRPHY3AO_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7))
#define DDRPHY3AO_DDRPHY_REG1_TOG_MASK ((0x1 << 0))
#define DDRPHY3AO_DDRPHY_REG1_TOG1 ((0x0 << 0))
#define DDRPHY3AO_DDRPHY_REG1_TOG0 ((0x1 << 0))
#if 0
static unsigned int ddrphy3ao_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DDRPHY3AO_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void ddrphy3ao_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DDRPHY3AO_MISC_CG_CTRL2,
		(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_ddrphy3ao_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DDRPHY3AO_MISC_CG_CTRL0) &
		DDRPHY3AO_DDRPHY_REG0_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_REG0_ON);
	ret &= ((reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
		DDRPHY3AO_DDRPHY_REG1_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_REG1_ON);

	return ret;
}

void dcm_ddrphy3ao_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy3ao_ddrphy'" */
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY3AO_DDRPHY_REG1_TOG0);
		ddrphy3ao_rg_mem_dcm_idle_fsel_set(0x1f);
		reg_write(DDRPHY3AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL0) &
			~DDRPHY3AO_DDRPHY_REG0_MASK) |
			DDRPHY3AO_DDRPHY_REG0_ON);
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_MASK) |
			DDRPHY3AO_DDRPHY_REG1_ON);
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY3AO_DDRPHY_REG1_TOG1);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy3ao_ddrphy'" */
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY3AO_DDRPHY_REG1_TOG0);
		ddrphy3ao_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DDRPHY3AO_MISC_CG_CTRL0,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL0) &
			~DDRPHY3AO_DDRPHY_REG0_MASK) |
			DDRPHY3AO_DDRPHY_REG0_OFF);
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_MASK) |
			DDRPHY3AO_DDRPHY_REG1_OFF);
		reg_write(DDRPHY3AO_MISC_CG_CTRL2,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL2) &
			~DDRPHY3AO_DDRPHY_REG1_TOG_MASK) |
			DDRPHY3AO_DDRPHY_REG1_TOG1);
	}
}

#define DDRPHY3AO_DDRPHY_E2_REG0_MASK ((0xffffffff << 0))
#define DDRPHY3AO_DDRPHY_E2_REG1_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY3AO_DDRPHY_E2_REG2_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG3_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG4_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG5_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG6_MASK ((0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG7_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG8_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG9_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG10_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG11_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG12_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG13_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG14_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG15_MASK ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG16_MASK ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG17_MASK ((0xffffffff << 0))
#define DDRPHY3AO_DDRPHY_E2_REG0_ON ((0x470000 << 0))
#define DDRPHY3AO_DDRPHY_E2_REG1_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DDRPHY3AO_DDRPHY_E2_REG2_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG3_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG4_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG5_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG6_ON ((0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG7_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG8_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG9_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG10_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG11_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG12_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG13_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG14_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG15_ON ((0x2 << 20))
#define DDRPHY3AO_DDRPHY_E2_REG16_ON ((0xf << 0) | \
			(0x1 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG17_ON ((0x2 << 0))
#define DDRPHY3AO_DDRPHY_E2_REG0_OFF ((0x0 << 0))
#define DDRPHY3AO_DDRPHY_E2_REG1_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DDRPHY3AO_DDRPHY_E2_REG2_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG3_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG4_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG5_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG6_OFF ((0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG7_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG8_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG9_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG10_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG11_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG12_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG13_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG14_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG15_OFF ((0xfff << 20))
#define DDRPHY3AO_DDRPHY_E2_REG16_OFF ((0x0 << 0) | \
			(0x0 << 16))
#define DDRPHY3AO_DDRPHY_E2_REG17_OFF ((0xfff << 0))

bool dcm_ddrphy3ao_ddrphy_e2_is_on(void)
{
	bool ret = true;

	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return false;

	ret &= ((reg_read(DDRPHY3AO_MISC_CG_CTRL5) &
		DDRPHY3AO_DDRPHY_E2_REG0_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG0_ON);
	ret &= ((reg_read(DDRPHY3AO_MISC_CTRL3) &
		DDRPHY3AO_DDRPHY_E2_REG1_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG1_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU1_B0_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG2_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG2_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0XC20) &
		DDRPHY3AO_DDRPHY_E2_REG3_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG3_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU1_B1_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG4_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG4_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0XCA0) &
		DDRPHY3AO_DDRPHY_E2_REG5_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG5_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU2_B0_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG6_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG6_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X1120) &
		DDRPHY3AO_DDRPHY_E2_REG7_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG7_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU2_B1_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG8_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG8_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X11A0) &
		DDRPHY3AO_DDRPHY_E2_REG9_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG9_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU3_B0_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG10_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG10_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X1620) &
		DDRPHY3AO_DDRPHY_E2_REG11_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG11_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU3_B1_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG12_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG12_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X16A0) &
		DDRPHY3AO_DDRPHY_E2_REG13_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG13_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU4_B0_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG14_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG14_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X1B20) &
		DDRPHY3AO_DDRPHY_E2_REG15_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG15_ON);
	ret &= ((reg_read(DDRPHY3AO_SHU4_B1_DQ0) &
		DDRPHY3AO_DDRPHY_E2_REG16_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG16_ON);
	ret &= ((reg_read(DDRPHY3AO_RFU_0X1BA0) &
		DDRPHY3AO_DDRPHY_E2_REG17_MASK) ==
		(unsigned int) DDRPHY3AO_DDRPHY_E2_REG17_ON);

	return ret;
}

void dcm_ddrphy3ao_ddrphy_e2(int on)
{
	if (dcm_chip_sw_ver < CHIP_SW_VER_02)
		return;

	if (on) {
		/* TINFO = "Turn ON DCM 'ddrphy3ao_ddrphy_e2'" */
		reg_write(DDRPHY3AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL5) &
			~DDRPHY3AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG0_ON);
		reg_write(DDRPHY3AO_MISC_CTRL3,
			(reg_read(DDRPHY3AO_MISC_CTRL3) &
			~DDRPHY3AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG1_ON);
		reg_write(DDRPHY3AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU1_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG2_ON);
		reg_write(DDRPHY3AO_RFU_0XC20,
			(reg_read(DDRPHY3AO_RFU_0XC20) &
			~DDRPHY3AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG3_ON);
		reg_write(DDRPHY3AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU1_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG4_ON);
		reg_write(DDRPHY3AO_RFU_0XCA0,
			(reg_read(DDRPHY3AO_RFU_0XCA0) &
			~DDRPHY3AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG5_ON);
		reg_write(DDRPHY3AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU2_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG6_ON);
		reg_write(DDRPHY3AO_RFU_0X1120,
			(reg_read(DDRPHY3AO_RFU_0X1120) &
			~DDRPHY3AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG7_ON);
		reg_write(DDRPHY3AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU2_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG8_ON);
		reg_write(DDRPHY3AO_RFU_0X11A0,
			(reg_read(DDRPHY3AO_RFU_0X11A0) &
			~DDRPHY3AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG9_ON);
		reg_write(DDRPHY3AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU3_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG10_ON);
		reg_write(DDRPHY3AO_RFU_0X1620,
			(reg_read(DDRPHY3AO_RFU_0X1620) &
			~DDRPHY3AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG11_ON);
		reg_write(DDRPHY3AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU3_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG12_ON);
		reg_write(DDRPHY3AO_RFU_0X16A0,
			(reg_read(DDRPHY3AO_RFU_0X16A0) &
			~DDRPHY3AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG13_ON);
		reg_write(DDRPHY3AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU4_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG14_ON);
		reg_write(DDRPHY3AO_RFU_0X1B20,
			(reg_read(DDRPHY3AO_RFU_0X1B20) &
			~DDRPHY3AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG15_ON);
		reg_write(DDRPHY3AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU4_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG16_ON);
		reg_write(DDRPHY3AO_RFU_0X1BA0,
			(reg_read(DDRPHY3AO_RFU_0X1BA0) &
			~DDRPHY3AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG17_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'ddrphy3ao_ddrphy_e2'" */
		reg_write(DDRPHY3AO_MISC_CG_CTRL5,
			(reg_read(DDRPHY3AO_MISC_CG_CTRL5) &
			~DDRPHY3AO_DDRPHY_E2_REG0_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG0_OFF);
		reg_write(DDRPHY3AO_MISC_CTRL3,
			(reg_read(DDRPHY3AO_MISC_CTRL3) &
			~DDRPHY3AO_DDRPHY_E2_REG1_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG1_OFF);
		reg_write(DDRPHY3AO_SHU1_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU1_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG2_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG2_OFF);
		reg_write(DDRPHY3AO_RFU_0XC20,
			(reg_read(DDRPHY3AO_RFU_0XC20) &
			~DDRPHY3AO_DDRPHY_E2_REG3_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG3_OFF);
		reg_write(DDRPHY3AO_SHU1_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU1_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG4_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG4_OFF);
		reg_write(DDRPHY3AO_RFU_0XCA0,
			(reg_read(DDRPHY3AO_RFU_0XCA0) &
			~DDRPHY3AO_DDRPHY_E2_REG5_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG5_OFF);
		reg_write(DDRPHY3AO_SHU2_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU2_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG6_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG6_OFF);
		reg_write(DDRPHY3AO_RFU_0X1120,
			(reg_read(DDRPHY3AO_RFU_0X1120) &
			~DDRPHY3AO_DDRPHY_E2_REG7_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG7_OFF);
		reg_write(DDRPHY3AO_SHU2_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU2_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG8_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG8_OFF);
		reg_write(DDRPHY3AO_RFU_0X11A0,
			(reg_read(DDRPHY3AO_RFU_0X11A0) &
			~DDRPHY3AO_DDRPHY_E2_REG9_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG9_OFF);
		reg_write(DDRPHY3AO_SHU3_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU3_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG10_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG10_OFF);
		reg_write(DDRPHY3AO_RFU_0X1620,
			(reg_read(DDRPHY3AO_RFU_0X1620) &
			~DDRPHY3AO_DDRPHY_E2_REG11_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG11_OFF);
		reg_write(DDRPHY3AO_SHU3_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU3_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG12_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG12_OFF);
		reg_write(DDRPHY3AO_RFU_0X16A0,
			(reg_read(DDRPHY3AO_RFU_0X16A0) &
			~DDRPHY3AO_DDRPHY_E2_REG13_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG13_OFF);
		reg_write(DDRPHY3AO_SHU4_B0_DQ0,
			(reg_read(DDRPHY3AO_SHU4_B0_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG14_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG14_OFF);
		reg_write(DDRPHY3AO_RFU_0X1B20,
			(reg_read(DDRPHY3AO_RFU_0X1B20) &
			~DDRPHY3AO_DDRPHY_E2_REG15_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG15_OFF);
		reg_write(DDRPHY3AO_SHU4_B1_DQ0,
			(reg_read(DDRPHY3AO_SHU4_B1_DQ0) &
			~DDRPHY3AO_DDRPHY_E2_REG16_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG16_OFF);
		reg_write(DDRPHY3AO_RFU_0X1BA0,
			(reg_read(DDRPHY3AO_RFU_0X1BA0) &
			~DDRPHY3AO_DDRPHY_E2_REG17_MASK) |
			DDRPHY3AO_DDRPHY_E2_REG17_OFF);
	}
}

#define DRAMC3_AO_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC3_AO_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC3_AO_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC3_AO_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC3_AO_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC3_AO_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc3_ao_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC3_AO_DRAMC_PD_CTRL) &
		DRAMC3_AO_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC3_AO_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC3_AO_CLKAR) &
		DRAMC3_AO_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC3_AO_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc3_ao_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc3_ao_dramc_dcm'" */
		reg_write(DRAMC3_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC3_AO_DRAMC_PD_CTRL) &
			~DRAMC3_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC3_AO_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC3_AO_CLKAR,
			(reg_read(DRAMC3_AO_CLKAR) &
			~DRAMC3_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC3_AO_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc3_ao_dramc_dcm'" */
		reg_write(DRAMC3_AO_DRAMC_PD_CTRL,
			(reg_read(DRAMC3_AO_DRAMC_PD_CTRL) &
			~DRAMC3_AO_DRAMC_DCM_REG0_MASK) |
			DRAMC3_AO_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC3_AO_CLKAR,
			(reg_read(DRAMC3_AO_CLKAR) &
			~DRAMC3_AO_DRAMC_DCM_REG1_MASK) |
			DRAMC3_AO_DRAMC_DCM_REG1_OFF);
	}
}
#endif /* #ifndef USE_DRAM_API_INSTEAD */

#define PERICFG_PERI_DCM_REG0_MASK ((0xfff << 0) | \
			(0x1 << 15) | \
			(0xfff << 16) | \
			(0x1 << 31))
#define PERICFG_PERI_DCM_REG1_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 9) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14))
#define PERICFG_PERI_DCM_REG2_MASK ((0xffffffff << 0))
#define PERICFG_PERI_DCM_REG0_ON ((0x1f << 0) | \
			(0x1 << 15) | \
			(0x1f << 16) | \
			(0x1 << 31))
#define PERICFG_PERI_DCM_REG1_ON ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 9) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14))
#define PERICFG_PERI_DCM_REG2_ON ((0x0 << 0))
#define PERICFG_PERI_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 31))
#define PERICFG_PERI_DCM_REG1_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 9) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14))
#define PERICFG_PERI_DCM_REG2_OFF ((0x0 << 0))

bool dcm_pericfg_peri_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERICFG_PERI_BIU_DBC_CTRL) &
		PERICFG_PERI_DCM_REG0_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_REG0_ON);
	ret &= ((reg_read(PERICFG_PERI_DCM_EMI_EARLY_CTRL) &
		PERICFG_PERI_DCM_REG1_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_REG1_ON);
	ret &= ((reg_read(PERICFG_PERI_DCM_REG_EARLY_CTRL) &
		PERICFG_PERI_DCM_REG2_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_REG2_ON);

	return ret;
}

void dcm_pericfg_peri_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pericfg_peri_dcm'" */
		reg_write(PERICFG_PERI_BIU_DBC_CTRL,
			(reg_read(PERICFG_PERI_BIU_DBC_CTRL) &
			~PERICFG_PERI_DCM_REG0_MASK) |
			PERICFG_PERI_DCM_REG0_ON);
		reg_write(PERICFG_PERI_DCM_EMI_EARLY_CTRL,
			(reg_read(PERICFG_PERI_DCM_EMI_EARLY_CTRL) &
			~PERICFG_PERI_DCM_REG1_MASK) |
			PERICFG_PERI_DCM_REG1_ON);
		reg_write(PERICFG_PERI_DCM_REG_EARLY_CTRL,
			(reg_read(PERICFG_PERI_DCM_REG_EARLY_CTRL) &
			~PERICFG_PERI_DCM_REG2_MASK) |
			PERICFG_PERI_DCM_REG2_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pericfg_peri_dcm'" */
		reg_write(PERICFG_PERI_BIU_DBC_CTRL,
			(reg_read(PERICFG_PERI_BIU_DBC_CTRL) &
			~PERICFG_PERI_DCM_REG0_MASK) |
			PERICFG_PERI_DCM_REG0_OFF);
		reg_write(PERICFG_PERI_DCM_EMI_EARLY_CTRL,
			(reg_read(PERICFG_PERI_DCM_EMI_EARLY_CTRL) &
			~PERICFG_PERI_DCM_REG1_MASK) |
			PERICFG_PERI_DCM_REG1_OFF);
		reg_write(PERICFG_PERI_DCM_REG_EARLY_CTRL,
			(reg_read(PERICFG_PERI_DCM_REG_EARLY_CTRL) &
			~PERICFG_PERI_DCM_REG2_MASK) |
			PERICFG_PERI_DCM_REG2_OFF);
	}
}

#define PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_MASK ((0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17))
#define PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_ON ((0x0 << 14) | \
			(0x1 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17))
#define PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_OFF ((0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17))
#if 0
static unsigned int pericfg_dcm_emi_group_biu_rg_fsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) >> 18) & 0x1f;
}
#endif
#if 0
static unsigned int pericfg_dcm_emi_group_biu_rg_sfsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) >> 23) & 0x1f;
}
#endif
static void pericfg_dcm_emi_group_biu_rg_fsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		~(0x1f << 18)) |
		(val & 0x1f) << 18);
}
static void pericfg_dcm_emi_group_biu_rg_sfsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		~(0x1f << 23)) |
		(val & 0x1f) << 23);
}

bool dcm_pericfg_peri_dcm_emi_group_biu_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_ON);

	return ret;
}

void dcm_pericfg_peri_dcm_emi_group_biu(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pericfg_peri_dcm_emi_group_biu'" */
		pericfg_dcm_emi_group_biu_rg_fsel_set(0x1f);
		pericfg_dcm_emi_group_biu_rg_sfsel_set(0x7);
		reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
			~PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_MASK) |
			PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pericfg_peri_dcm_emi_group_biu'" */
		pericfg_dcm_emi_group_biu_rg_fsel_set(0x1f);
		pericfg_dcm_emi_group_biu_rg_sfsel_set(0x7);
		reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
			~PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_MASK) |
			PERICFG_PERI_DCM_EMI_GROUP_BIU_REG0_OFF);
	}
}

#define PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#define PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#if 0
static unsigned int pericfg_dcm_emi_group_bus_rg_fsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) >> 4) & 0x1f;
}
#endif
#if 0
static unsigned int pericfg_dcm_emi_group_bus_rg_sfsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) >> 9) & 0x1f;
}
#endif
static void pericfg_dcm_emi_group_bus_rg_fsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		~(0x1f << 4)) |
		(val & 0x1f) << 4);
}
static void pericfg_dcm_emi_group_bus_rg_sfsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		~(0x1f << 9)) |
		(val & 0x1f) << 9);
}

bool dcm_pericfg_peri_dcm_emi_group_bus_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
		PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_ON);

	return ret;
}

void dcm_pericfg_peri_dcm_emi_group_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pericfg_peri_dcm_emi_group_bus'" */
		pericfg_dcm_emi_group_bus_rg_fsel_set(0x1f);
		pericfg_dcm_emi_group_bus_rg_sfsel_set(0x1f);
		reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
			~PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_MASK) |
			PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pericfg_peri_dcm_emi_group_bus'" */
		pericfg_dcm_emi_group_bus_rg_fsel_set(0x1f);
		pericfg_dcm_emi_group_bus_rg_sfsel_set(0x1f);
		reg_write(PERICFG_PERI_BIU_EMI_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_EMI_DCM_CTRL) &
			~PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_MASK) |
			PERICFG_PERI_DCM_EMI_GROUP_BUS_REG0_OFF);
	}
}

#define PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_MASK ((0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17))
#define PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_ON ((0x1 << 14) | \
			(0x1 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17))
#define PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_OFF ((0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17))
#if 0
static unsigned int pericfg_dcm_reg_group_biu_rg_fsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) >> 18) & 0x1f;
}
#endif
#if 0
static unsigned int pericfg_dcm_reg_group_biu_rg_sfsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) >> 23) & 0x1f;
}
#endif
static void pericfg_dcm_reg_group_biu_rg_fsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		~(0x1f << 18)) |
		(val & 0x1f) << 18);
}
static void pericfg_dcm_reg_group_biu_rg_sfsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		~(0x1f << 23)) |
		(val & 0x1f) << 23);
}

bool dcm_pericfg_peri_dcm_reg_group_biu_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_ON);

	return ret;
}

void dcm_pericfg_peri_dcm_reg_group_biu(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pericfg_peri_dcm_reg_group_biu'" */
		pericfg_dcm_reg_group_biu_rg_fsel_set(0x1f);
		pericfg_dcm_reg_group_biu_rg_sfsel_set(0xf);
		reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
			~PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_MASK) |
			PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pericfg_peri_dcm_reg_group_biu'" */
		pericfg_dcm_reg_group_biu_rg_fsel_set(0x1f);
		pericfg_dcm_reg_group_biu_rg_sfsel_set(0xf);
		reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
			~PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_MASK) |
			PERICFG_PERI_DCM_REG_GROUP_BIU_REG0_OFF);
	}
}

#define PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3))
#define PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#define PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3))
#if 0
static unsigned int pericfg_dcm_reg_group_bus_rg_fsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) >> 4) & 0x1f;
}
#endif
#if 0
static unsigned int pericfg_dcm_reg_group_bus_rg_sfsel_get(void)
{
	return (reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) >> 9) & 0x1f;
}
#endif
static void pericfg_dcm_reg_group_bus_rg_fsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		~(0x1f << 4)) |
		(val & 0x1f) << 4);
}
static void pericfg_dcm_reg_group_bus_rg_sfsel_set(unsigned int val)
{
	reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
		(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		~(0x1f << 9)) |
		(val & 0x1f) << 9);
}

bool dcm_pericfg_peri_dcm_reg_group_bus_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
		PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_MASK) ==
		(unsigned int) PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_ON);

	return ret;
}

void dcm_pericfg_peri_dcm_reg_group_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'pericfg_peri_dcm_reg_group_bus'" */
		pericfg_dcm_reg_group_bus_rg_fsel_set(0x1f);
		pericfg_dcm_reg_group_bus_rg_sfsel_set(0xf);
		reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
			~PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_MASK) |
			PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'pericfg_peri_dcm_reg_group_bus'" */
		pericfg_dcm_reg_group_bus_rg_fsel_set(0x1f);
		pericfg_dcm_reg_group_bus_rg_sfsel_set(0xf);
		reg_write(PERICFG_PERI_BIU_REG_DCM_CTRL,
			(reg_read(PERICFG_PERI_BIU_REG_DCM_CTRL) &
			~PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_MASK) |
			PERICFG_PERI_DCM_REG_GROUP_BUS_REG0_OFF);
	}
}

#define MCSI_REG_CCI_CACTIVE_REG0_MASK ((0xffffffff << 0))
#define MCSI_REG_CCI_CACTIVE_REG0_ON ((0x0 << 0))
#define MCSI_REG_CCI_CACTIVE_REG0_OFF ((0xffffffff << 0))

bool dcm_mcsi_reg_cci_cactive_is_on(void)
{
	bool ret = true;

	ret &= ((MCSI_SMC_READ(CCI_CACTIVE) &
		MCSI_REG_CCI_CACTIVE_REG0_MASK) ==
		(unsigned int) MCSI_REG_CCI_CACTIVE_REG0_ON);

	return ret;
}

void dcm_mcsi_reg_cci_cactive(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcsi_reg_cci_cactive'" */
		MCSI_SMC_WRITE(CCI_CACTIVE,
			(MCSI_SMC_READ(CCI_CACTIVE) &
			~MCSI_REG_CCI_CACTIVE_REG0_MASK) |
			MCSI_REG_CCI_CACTIVE_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcsi_reg_cci_cactive'" */
		MCSI_SMC_WRITE(CCI_CACTIVE,
			(MCSI_SMC_READ(CCI_CACTIVE) &
			~MCSI_REG_CCI_CACTIVE_REG0_MASK) |
			MCSI_REG_CCI_CACTIVE_REG0_OFF);
	}
}

#define MCSI_REG_CCI_DCM_REG0_MASK ((0xffffffff << 0))
#define MCSI_REG_CCI_DCM_REG0_ON ((0x0 << 0))
#define MCSI_REG_CCI_DCM_REG0_OFF ((0xffffffff << 0))

bool dcm_mcsi_reg_cci_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((MCSI_SMC_READ(CCI_DCM) &
		MCSI_REG_CCI_DCM_REG0_MASK) ==
		(unsigned int) MCSI_REG_CCI_DCM_REG0_ON);

	return ret;
}

void dcm_mcsi_reg_cci_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcsi_reg_cci_dcm'" */
		MCSI_SMC_WRITE(CCI_DCM,
			(MCSI_SMC_READ(CCI_DCM) &
			~MCSI_REG_CCI_DCM_REG0_MASK) |
			MCSI_REG_CCI_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcsi_reg_cci_dcm'" */
		MCSI_SMC_WRITE(CCI_DCM,
			(MCSI_SMC_READ(CCI_DCM) &
			~MCSI_REG_CCI_DCM_REG0_MASK) |
			MCSI_REG_CCI_DCM_REG0_OFF);
	}
}

