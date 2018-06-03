/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
/* #include <mt-plat/mtk_secure_api.h> */

#include <mtk_dcm_internal.h>
#include <mtk_dcm_autogen.h>
#include <mtk_dcm.h>

#define INFRACFG_AO_INFRA_BUS_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1f << 5) | \
			(0x1 << 20) | \
			(0x1 << 23) | \
			(0x1 << 30))
#define INFRACFG_AO_INFRA_BUS_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x10 << 5) | \
			(0x1 << 20) | \
			(0x1 << 23) | \
			(0x1 << 30))
#define INFRACFG_AO_INFRA_BUS_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x10 << 5) | \
			(0x0 << 20) | \
			(0x0 << 23) | \
			(0x0 << 30))
#if 0
static unsigned int infracfg_ao_infra_dcm_rg_sfsel_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
static void infracfg_ao_infra_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}

bool dcm_infracfg_ao_infra_bus_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(INFRA_BUS_DCM_CTRL) &
		INFRACFG_AO_INFRA_BUS_DCM_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_INFRA_BUS_DCM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_infra_bus_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_infra_bus_dcm'" */
		infracfg_ao_infra_dcm_rg_sfsel_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL,
			(reg_read(INFRA_BUS_DCM_CTRL) &
			~INFRACFG_AO_INFRA_BUS_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_BUS_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_infra_bus_dcm'" */
		infracfg_ao_infra_dcm_rg_sfsel_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL,
			(reg_read(INFRA_BUS_DCM_CTRL) &
			~INFRACFG_AO_INFRA_BUS_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_BUS_DCM_REG0_OFF);
	}
}

#define INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_MASK ((0x1 << 27))
#define INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_ON ((0x1 << 27))
#define INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_OFF ((0x0 << 27))

bool dcm_infracfg_ao_infra_emi_local_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MEM_DCM_CTRL) &
		INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_infra_emi_local_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_infra_emi_local_dcm'" */
		reg_write(MEM_DCM_CTRL,
			(reg_read(MEM_DCM_CTRL) &
			~INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_infra_emi_local_dcm'" */
		reg_write(MEM_DCM_CTRL,
			(reg_read(MEM_DCM_CTRL) &
			~INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_EMI_LOCAL_DCM_REG0_OFF);
	}
}

#define INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_MASK ((0xf << 0))
#define INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_ON ((0x0 << 0))
#define INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_OFF ((0xf << 0))

bool dcm_infracfg_ao_infra_rx_p2p_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(P2P_RX_CLK_ON) &
		INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_infra_rx_p2p_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_infra_rx_p2p_dcm'" */
		reg_write(P2P_RX_CLK_ON,
			(reg_read(P2P_RX_CLK_ON) &
			~INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_infra_rx_p2p_dcm'" */
		reg_write(P2P_RX_CLK_ON,
			(reg_read(P2P_RX_CLK_ON) &
			~INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_MASK) |
			INFRACFG_AO_INFRA_RX_P2P_DCM_REG0_OFF);
	}
}

#define INFRACFG_AO_PERI_BUS_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1f << 5) | \
			(0x1f << 15) | \
			(0x1 << 20) | \
			(0x1 << 21))
#define INFRACFG_AO_PERI_BUS_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x1f << 5) | \
			(0x1f << 15) | \
			(0x1 << 20) | \
			(0x1 << 21))
#define INFRACFG_AO_PERI_BUS_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x1f << 5) | \
			(0x0 << 15) | \
			(0x0 << 20) | \
			(0x0 << 21))
#if 0
static unsigned int infracfg_ao_peri_dcm_rg_sfsel_get(void)
{
	return (reg_read(PERI_BUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
static void infracfg_ao_peri_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(PERI_BUS_DCM_CTRL,
		(reg_read(PERI_BUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}

bool dcm_infracfg_ao_peri_bus_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERI_BUS_DCM_CTRL) &
		INFRACFG_AO_PERI_BUS_DCM_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_PERI_BUS_DCM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_peri_bus_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_peri_bus_dcm'" */
		infracfg_ao_peri_dcm_rg_sfsel_set(0x0);
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_PERI_BUS_DCM_REG0_MASK) |
			INFRACFG_AO_PERI_BUS_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_peri_bus_dcm'" */
		infracfg_ao_peri_dcm_rg_sfsel_set(0x1f);
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_PERI_BUS_DCM_REG0_MASK) |
			INFRACFG_AO_PERI_BUS_DCM_REG0_OFF);
	}
}

#define INFRACFG_AO_PERI_MODULE_DCM_REG0_MASK ((0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30))
#define INFRACFG_AO_PERI_MODULE_DCM_REG0_ON ((0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30))
#define INFRACFG_AO_PERI_MODULE_DCM_REG0_OFF ((0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30))

bool dcm_infracfg_ao_peri_module_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(PERI_BUS_DCM_CTRL) &
		INFRACFG_AO_PERI_MODULE_DCM_REG0_MASK) ==
		(unsigned int) INFRACFG_AO_PERI_MODULE_DCM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_peri_module_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_peri_module_dcm'" */
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_PERI_MODULE_DCM_REG0_MASK) |
			INFRACFG_AO_PERI_MODULE_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_peri_module_dcm'" */
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_PERI_MODULE_DCM_REG0_MASK) |
			INFRACFG_AO_PERI_MODULE_DCM_REG0_OFF);
	}
}

#define EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define EMI_DCM_EMI_GROUP_REG1_MASK ((0xff << 24))
#define EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define EMI_DCM_EMI_GROUP_REG1_ON ((0x0 << 24))
#define EMI_DCM_EMI_GROUP_REG0_OFF ((0xff << 24))
#define EMI_DCM_EMI_GROUP_REG1_OFF ((0xff << 24))

bool dcm_emi_dcm_emi_group_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(EMI_CONM) &
		EMI_DCM_EMI_GROUP_REG0_MASK) ==
		(unsigned int) EMI_DCM_EMI_GROUP_REG0_ON);
	ret &= ((reg_read(EMI_CONN) &
		EMI_DCM_EMI_GROUP_REG1_MASK) ==
		(unsigned int) EMI_DCM_EMI_GROUP_REG1_ON);

	return ret;
}

void dcm_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'emi_dcm_emi_group'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_DCM_EMI_GROUP_REG0_MASK) |
			EMI_DCM_EMI_GROUP_REG0_ON);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_DCM_EMI_GROUP_REG1_MASK) |
			EMI_DCM_EMI_GROUP_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'emi_dcm_emi_group'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_DCM_EMI_GROUP_REG0_MASK) |
			EMI_DCM_EMI_GROUP_REG0_OFF);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_DCM_EMI_GROUP_REG1_MASK) |
			EMI_DCM_EMI_GROUP_REG1_OFF);
	}
}

#define DRAMC_CH0_TOP0_DDRPHY_REG0_MASK ((0x1 << 8) | \
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
#define DRAMC_CH0_TOP0_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 26))
#define DRAMC_CH0_TOP0_DDRPHY_REG2_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DRAMC_CH0_TOP0_DDRPHY_REG3_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG4_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG5_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG6_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG7_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG8_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG9_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG10_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG11_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG12_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG13_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG14_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG0_ON ((0x0 << 8) | \
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
#define DRAMC_CH0_TOP0_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 26))
#define DRAMC_CH0_TOP0_DDRPHY_REG2_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DRAMC_CH0_TOP0_DDRPHY_REG3_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG4_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG5_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG6_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG7_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG8_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG9_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG10_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG11_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG12_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG13_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG14_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG0_OFF ((0x1 << 8) | \
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
#define DRAMC_CH0_TOP0_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x0 << 26))
#define DRAMC_CH0_TOP0_DDRPHY_REG2_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DRAMC_CH0_TOP0_DDRPHY_REG3_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG4_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG5_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG6_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG7_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG8_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG9_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG10_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG11_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG12_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG13_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP0_DDRPHY_REG14_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#if 0
static unsigned int dramc_ch0_top0_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
		(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_dramc_ch0_top0_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
		DRAMC_CH0_TOP0_DDRPHY_REG0_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG0_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
		DRAMC_CH0_TOP0_DDRPHY_REG1_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG1_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_MISC_CTRL3) &
		DRAMC_CH0_TOP0_DDRPHY_REG2_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG2_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU1_B0_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG3_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG3_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU1_B1_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG4_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG4_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU1_CA_CMD8) &
		DRAMC_CH0_TOP0_DDRPHY_REG5_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG5_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU2_B0_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG6_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG6_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU2_B1_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG7_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG7_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU2_CA_CMD8) &
		DRAMC_CH0_TOP0_DDRPHY_REG8_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG8_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU3_B0_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG9_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG9_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU3_B1_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG10_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG10_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU3_CA_CMD8) &
		DRAMC_CH0_TOP0_DDRPHY_REG11_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG11_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU4_B0_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG12_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG12_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU4_B1_DQ8) &
		DRAMC_CH0_TOP0_DDRPHY_REG13_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG13_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP0_SHU4_CA_CMD8) &
		DRAMC_CH0_TOP0_DDRPHY_REG14_MASK) ==
		(unsigned int) DRAMC_CH0_TOP0_DDRPHY_REG14_ON);

	return ret;
}

void dcm_dramc_ch0_top0_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch0_top0_ddrphy'" */
		dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(0x8);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH0_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG0_ON);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH0_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG1_ON);
		reg_write(DRAMC_CH0_TOP0_MISC_CTRL3,
			(reg_read(DRAMC_CH0_TOP0_MISC_CTRL3) &
			~DRAMC_CH0_TOP0_DDRPHY_REG2_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG2_ON);
		reg_write(DRAMC_CH0_TOP0_SHU1_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG3_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG3_ON);
		reg_write(DRAMC_CH0_TOP0_SHU1_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG4_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG4_ON);
		reg_write(DRAMC_CH0_TOP0_SHU1_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG5_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG5_ON);
		reg_write(DRAMC_CH0_TOP0_SHU2_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG6_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG6_ON);
		reg_write(DRAMC_CH0_TOP0_SHU2_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG7_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG7_ON);
		reg_write(DRAMC_CH0_TOP0_SHU2_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG8_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG8_ON);
		reg_write(DRAMC_CH0_TOP0_SHU3_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG9_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG9_ON);
		reg_write(DRAMC_CH0_TOP0_SHU3_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG10_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG10_ON);
		reg_write(DRAMC_CH0_TOP0_SHU3_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG11_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG11_ON);
		reg_write(DRAMC_CH0_TOP0_SHU4_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG12_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG12_ON);
		reg_write(DRAMC_CH0_TOP0_SHU4_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG13_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG13_ON);
		reg_write(DRAMC_CH0_TOP0_SHU4_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG14_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG14_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch0_top0_ddrphy'" */
		dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH0_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG0_OFF);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH0_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG1_OFF);
		reg_write(DRAMC_CH0_TOP0_MISC_CTRL3,
			(reg_read(DRAMC_CH0_TOP0_MISC_CTRL3) &
			~DRAMC_CH0_TOP0_DDRPHY_REG2_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG2_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU1_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG3_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG3_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU1_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG4_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG4_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU1_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU1_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG5_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG5_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU2_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG6_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG6_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU2_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG7_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG7_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU2_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU2_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG8_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG8_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU3_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG9_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG9_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU3_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG10_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG10_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU3_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU3_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG11_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG11_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU4_B0_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_B0_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG12_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG12_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU4_B1_DQ8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_B1_DQ8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG13_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG13_OFF);
		reg_write(DRAMC_CH0_TOP0_SHU4_CA_CMD8,
			(reg_read(DRAMC_CH0_TOP0_SHU4_CA_CMD8) &
			~DRAMC_CH0_TOP0_DDRPHY_REG14_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG14_OFF);
	}
}

#define DRAMC_CH0_TOP1_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP1_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC_CH0_TOP1_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP1_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC_CH0_TOP1_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP1_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc_ch0_top1_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
		DRAMC_CH0_TOP1_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC_CH0_TOP1_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC_CH0_TOP1_CLKAR) &
		DRAMC_CH0_TOP1_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC_CH0_TOP1_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc_ch0_top1_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch0_top1_dramc_dcm'" */
		reg_write(DRAMC_CH0_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH0_TOP1_DRAMC_DCM_REG0_MASK) |
			DRAMC_CH0_TOP1_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC_CH0_TOP1_CLKAR,
			(reg_read(DRAMC_CH0_TOP1_CLKAR) &
			~DRAMC_CH0_TOP1_DRAMC_DCM_REG1_MASK) |
			DRAMC_CH0_TOP1_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch0_top1_dramc_dcm'" */
		reg_write(DRAMC_CH0_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH0_TOP1_DRAMC_DCM_REG0_MASK) |
			DRAMC_CH0_TOP1_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC_CH0_TOP1_CLKAR,
			(reg_read(DRAMC_CH0_TOP1_CLKAR) &
			~DRAMC_CH0_TOP1_DRAMC_DCM_REG1_MASK) |
			DRAMC_CH0_TOP1_DRAMC_DCM_REG1_OFF);
	}
}

#define CHN0_EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define CHN0_EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define CHN0_EMI_DCM_EMI_GROUP_REG0_OFF ((0xff << 24))

bool dcm_chn0_emi_dcm_emi_group_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CHN0_EMI_CHN_EMI_CONB) &
		CHN0_EMI_DCM_EMI_GROUP_REG0_MASK) ==
		(unsigned int) CHN0_EMI_DCM_EMI_GROUP_REG0_ON);

	return ret;
}

void dcm_chn0_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'chn0_emi_dcm_emi_group'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN0_EMI_DCM_EMI_GROUP_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn0_emi_dcm_emi_group'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN0_EMI_DCM_EMI_GROUP_REG0_OFF);
	}
}

#define DRAMC_CH1_TOP0_DDRPHY_REG0_MASK ((0x1 << 8) | \
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
#define DRAMC_CH1_TOP0_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 26))
#define DRAMC_CH1_TOP0_DDRPHY_REG2_MASK ((0x1 << 26) | \
			(0x1 << 27))
#define DRAMC_CH1_TOP0_DDRPHY_REG3_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG4_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG5_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG6_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG7_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG8_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG9_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG10_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG11_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG12_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG13_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG14_MASK ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG0_ON ((0x0 << 8) | \
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
#define DRAMC_CH1_TOP0_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 26))
#define DRAMC_CH1_TOP0_DDRPHY_REG2_ON ((0x0 << 26) | \
			(0x0 << 27))
#define DRAMC_CH1_TOP0_DDRPHY_REG3_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG4_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG5_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG6_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG7_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG8_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG9_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG10_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG11_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG12_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG13_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG14_ON ((0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 22) | \
			(0x0 << 23) | \
			(0x0 << 24) | \
			(0x0 << 25) | \
			(0x0 << 26) | \
			(0x0 << 27) | \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG0_OFF ((0x1 << 8) | \
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
#define DRAMC_CH1_TOP0_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x0 << 26))
#define DRAMC_CH1_TOP0_DDRPHY_REG2_OFF ((0x1 << 26) | \
			(0x1 << 27))
#define DRAMC_CH1_TOP0_DDRPHY_REG3_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG4_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG5_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG6_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG7_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG8_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG9_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG10_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG11_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG12_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG13_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP0_DDRPHY_REG14_OFF ((0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 22) | \
			(0x1 << 23) | \
			(0x1 << 24) | \
			(0x1 << 25) | \
			(0x1 << 26) | \
			(0x1 << 27) | \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 30) | \
			(0x1 << 31))
#if 0
static unsigned int dramc_ch1_top0_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
		(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_dramc_ch1_top0_ddrphy_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
		DRAMC_CH1_TOP0_DDRPHY_REG0_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG0_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
		DRAMC_CH1_TOP0_DDRPHY_REG1_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG1_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_MISC_CTRL3) &
		DRAMC_CH1_TOP0_DDRPHY_REG2_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG2_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU1_B0_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG3_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG3_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU1_B1_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG4_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG4_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU1_CA_CMD8) &
		DRAMC_CH1_TOP0_DDRPHY_REG5_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG5_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU2_B0_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG6_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG6_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU2_B1_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG7_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG7_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU2_CA_CMD8) &
		DRAMC_CH1_TOP0_DDRPHY_REG8_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG8_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU3_B0_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG9_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG9_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU3_B1_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG10_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG10_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU3_CA_CMD8) &
		DRAMC_CH1_TOP0_DDRPHY_REG11_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG11_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU4_B0_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG12_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG12_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU4_B1_DQ8) &
		DRAMC_CH1_TOP0_DDRPHY_REG13_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG13_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP0_SHU4_CA_CMD8) &
		DRAMC_CH1_TOP0_DDRPHY_REG14_MASK) ==
		(unsigned int) DRAMC_CH1_TOP0_DDRPHY_REG14_ON);

	return ret;
}

void dcm_dramc_ch1_top0_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch1_top0_ddrphy'" */
		dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(0x8);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH1_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG0_ON);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH1_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG1_ON);
		reg_write(DRAMC_CH1_TOP0_MISC_CTRL3,
			(reg_read(DRAMC_CH1_TOP0_MISC_CTRL3) &
			~DRAMC_CH1_TOP0_DDRPHY_REG2_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG2_ON);
		reg_write(DRAMC_CH1_TOP0_SHU1_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG3_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG3_ON);
		reg_write(DRAMC_CH1_TOP0_SHU1_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG4_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG4_ON);
		reg_write(DRAMC_CH1_TOP0_SHU1_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG5_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG5_ON);
		reg_write(DRAMC_CH1_TOP0_SHU2_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG6_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG6_ON);
		reg_write(DRAMC_CH1_TOP0_SHU2_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG7_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG7_ON);
		reg_write(DRAMC_CH1_TOP0_SHU2_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG8_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG8_ON);
		reg_write(DRAMC_CH1_TOP0_SHU3_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG9_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG9_ON);
		reg_write(DRAMC_CH1_TOP0_SHU3_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG10_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG10_ON);
		reg_write(DRAMC_CH1_TOP0_SHU3_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG11_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG11_ON);
		reg_write(DRAMC_CH1_TOP0_SHU4_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG12_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG12_ON);
		reg_write(DRAMC_CH1_TOP0_SHU4_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG13_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG13_ON);
		reg_write(DRAMC_CH1_TOP0_SHU4_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG14_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG14_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch1_top0_ddrphy'" */
		dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH1_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG0_OFF);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH1_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG1_OFF);
		reg_write(DRAMC_CH1_TOP0_MISC_CTRL3,
			(reg_read(DRAMC_CH1_TOP0_MISC_CTRL3) &
			~DRAMC_CH1_TOP0_DDRPHY_REG2_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG2_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU1_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG3_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG3_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU1_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG4_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG4_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU1_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU1_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG5_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG5_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU2_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG6_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG6_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU2_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG7_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG7_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU2_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU2_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG8_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG8_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU3_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG9_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG9_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU3_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG10_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG10_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU3_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU3_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG11_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG11_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU4_B0_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_B0_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG12_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG12_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU4_B1_DQ8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_B1_DQ8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG13_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG13_OFF);
		reg_write(DRAMC_CH1_TOP0_SHU4_CA_CMD8,
			(reg_read(DRAMC_CH1_TOP0_SHU4_CA_CMD8) &
			~DRAMC_CH1_TOP0_DDRPHY_REG14_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG14_OFF);
	}
}

#define DRAMC_CH1_TOP1_DRAMC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP1_DRAMC_DCM_REG1_MASK ((0x1 << 31))
#define DRAMC_CH1_TOP1_DRAMC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP1_DRAMC_DCM_REG1_ON ((0x1 << 31))
#define DRAMC_CH1_TOP1_DRAMC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP1_DRAMC_DCM_REG1_OFF ((0x0 << 31))

bool dcm_dramc_ch1_top1_dramc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
		DRAMC_CH1_TOP1_DRAMC_DCM_REG0_MASK) ==
		(unsigned int) DRAMC_CH1_TOP1_DRAMC_DCM_REG0_ON);
	ret &= ((reg_read(DRAMC_CH1_TOP1_CLKAR) &
		DRAMC_CH1_TOP1_DRAMC_DCM_REG1_MASK) ==
		(unsigned int) DRAMC_CH1_TOP1_DRAMC_DCM_REG1_ON);

	return ret;
}

void dcm_dramc_ch1_top1_dramc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch1_top1_dramc_dcm'" */
		reg_write(DRAMC_CH1_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH1_TOP1_DRAMC_DCM_REG0_MASK) |
			DRAMC_CH1_TOP1_DRAMC_DCM_REG0_ON);
		reg_write(DRAMC_CH1_TOP1_CLKAR,
			(reg_read(DRAMC_CH1_TOP1_CLKAR) &
			~DRAMC_CH1_TOP1_DRAMC_DCM_REG1_MASK) |
			DRAMC_CH1_TOP1_DRAMC_DCM_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch1_top1_dramc_dcm'" */
		reg_write(DRAMC_CH1_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH1_TOP1_DRAMC_DCM_REG0_MASK) |
			DRAMC_CH1_TOP1_DRAMC_DCM_REG0_OFF);
		reg_write(DRAMC_CH1_TOP1_CLKAR,
			(reg_read(DRAMC_CH1_TOP1_CLKAR) &
			~DRAMC_CH1_TOP1_DRAMC_DCM_REG1_MASK) |
			DRAMC_CH1_TOP1_DRAMC_DCM_REG1_OFF);
	}
}

#define CHN1_EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define CHN1_EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define CHN1_EMI_DCM_EMI_GROUP_REG0_OFF ((0xff << 24))

bool dcm_chn1_emi_dcm_emi_group_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CHN1_EMI_CHN_EMI_CONB) &
		CHN1_EMI_DCM_EMI_GROUP_REG0_MASK) ==
		(unsigned int) CHN1_EMI_DCM_EMI_GROUP_REG0_ON);

	return ret;
}

void dcm_chn1_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'chn1_emi_dcm_emi_group'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN1_EMI_DCM_EMI_GROUP_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn1_emi_dcm_emi_group'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN1_EMI_DCM_EMI_GROUP_REG0_OFF);
	}
}

#define MP0_CPUCFG_MP0_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MP0_CPUCFG_MP0_RGU_DCM_REG0_ON ((0x1 << 0))
#define MP0_CPUCFG_MP0_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mp0_cpucfg_mp0_rgu_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_CPUCFG_MP0_RGU_DCM_CONFIG) &
		MP0_CPUCFG_MP0_RGU_DCM_REG0_MASK) ==
		(unsigned int) MP0_CPUCFG_MP0_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mp0_cpucfg_mp0_rgu_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mp0_cpucfg_mp0_rgu_dcm'" */
		reg_write(MP0_CPUCFG_MP0_RGU_DCM_CONFIG,
			(reg_read(MP0_CPUCFG_MP0_RGU_DCM_CONFIG) &
			~MP0_CPUCFG_MP0_RGU_DCM_REG0_MASK) |
			MP0_CPUCFG_MP0_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mp0_cpucfg_mp0_rgu_dcm'" */
		reg_write(MP0_CPUCFG_MP0_RGU_DCM_CONFIG,
			(reg_read(MP0_CPUCFG_MP0_RGU_DCM_CONFIG) &
			~MP0_CPUCFG_MP0_RGU_DCM_REG0_MASK) |
			MP0_CPUCFG_MP0_RGU_DCM_REG0_OFF);
	}
}

#define MP1_CPUCFG_MP1_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MP1_CPUCFG_MP1_RGU_DCM_REG0_ON ((0x1 << 0))
#define MP1_CPUCFG_MP1_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mp1_cpucfg_mp1_rgu_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP1_CPUCFG_MP1_RGU_DCM_CONFIG) &
		MP1_CPUCFG_MP1_RGU_DCM_REG0_MASK) ==
		(unsigned int) MP1_CPUCFG_MP1_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mp1_cpucfg_mp1_rgu_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mp1_cpucfg_mp1_rgu_dcm'" */
		reg_write(MP1_CPUCFG_MP1_RGU_DCM_CONFIG,
			(reg_read(MP1_CPUCFG_MP1_RGU_DCM_CONFIG) &
			~MP1_CPUCFG_MP1_RGU_DCM_REG0_MASK) |
			MP1_CPUCFG_MP1_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mp1_cpucfg_mp1_rgu_dcm'" */
		reg_write(MP1_CPUCFG_MP1_RGU_DCM_CONFIG,
			(reg_read(MP1_CPUCFG_MP1_RGU_DCM_CONFIG) &
			~MP1_CPUCFG_MP1_RGU_DCM_REG0_MASK) |
			MP1_CPUCFG_MP1_RGU_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_ADB400_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 5) | \
			(0x1 << 6))
#define MCU_MISCCFG_ADB400_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 5) | \
			(0x1 << 6))
#define MCU_MISCCFG_ADB400_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 5) | \
			(0x0 << 6))

bool dcm_mcu_misccfg_adb400_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CCI_ADB400_DCM_CONFIG) &
		MCU_MISCCFG_ADB400_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_ADB400_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_adb400_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_adb400_dcm'" */
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCU_MISCCFG_ADB400_DCM_REG0_MASK) |
			MCU_MISCCFG_ADB400_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_adb400_dcm'" */
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCU_MISCCFG_ADB400_DCM_REG0_MASK) |
			MCU_MISCCFG_ADB400_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcu_misccfg_bus_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(BUS_PLL_DIVIDER_CFG) &
		MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_bus_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_bus_arm_pll_divider_dcm'" */
		reg_write(BUS_PLL_DIVIDER_CFG,
			(reg_read(BUS_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_bus_arm_pll_divider_dcm'" */
		reg_write(BUS_PLL_DIVIDER_CFG,
			(reg_read(BUS_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_OFF ((0x0 << 0))
#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG_MASK ((0x1 << 1))
#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG1 ((0x1 << 1))
#define MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG0 ((0x1 << 1))
#if 0
static unsigned int mcu_misccfg_cci_sync_dcm_div_sel_get(void)
{
	return (reg_read(SYNC_DCM_CONFIG) >> 2) & 0x7f;
}
static void mcu_misccfg_cci_sync_dcm_div_sel_set(unsigned int val)
{
	reg_write(SYNC_DCM_CONFIG,
		(reg_read(SYNC_DCM_CONFIG) &
		~(0x7f << 2)) |
		(val & 0x7f) << 2);
}
#endif

bool dcm_mcu_misccfg_bus_sync_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CONFIG) &
		MCU_MISCCFG_BUS_SYNC_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_BUS_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_bus_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_bus_sync_dcm'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG0);
		mcu_misccfg_cci_sync_dcm_div_sel_set(0x0);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_ON);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG1);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_bus_sync_dcm'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG0);
		mcu_misccfg_cci_sync_dcm_div_sel_set(0x1);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_OFF);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG_MASK) |
			MCU_MISCCFG_BUS_SYNC_DCM_REG0_TOG1);
#endif
	}
}

#define MCU_MISCCFG_BUS_CLOCK_DCM_REG0_MASK ((0x1 << 8))
#define MCU_MISCCFG_BUS_CLOCK_DCM_REG0_ON ((0x1 << 8))
#define MCU_MISCCFG_BUS_CLOCK_DCM_REG0_OFF ((0x0 << 8))

bool dcm_mcu_misccfg_bus_clock_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(CCI_CLK_CTRL) &
		MCU_MISCCFG_BUS_CLOCK_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_BUS_CLOCK_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_bus_clock_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_bus_clock_dcm'" */
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCU_MISCCFG_BUS_CLOCK_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_CLOCK_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_bus_clock_dcm'" */
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCU_MISCCFG_BUS_CLOCK_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_CLOCK_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_BUS_FABRIC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
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
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 22) | \
			(0x1 << 23))
#define MCU_MISCCFG_BUS_FABRIC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
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
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 22) | \
			(0x1 << 23))
#define MCU_MISCCFG_BUS_FABRIC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
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
			(0x0 << 18) | \
			(0x0 << 19) | \
			(0x0 << 22) | \
			(0x0 << 23))

bool dcm_mcu_misccfg_bus_fabric_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(BUS_FABRIC_DCM_CTRL) &
		MCU_MISCCFG_BUS_FABRIC_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_BUS_FABRIC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_bus_fabric_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_bus_fabric_dcm'" */
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCU_MISCCFG_BUS_FABRIC_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_FABRIC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_bus_fabric_dcm'" */
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCU_MISCCFG_BUS_FABRIC_DCM_REG0_MASK) |
			MCU_MISCCFG_BUS_FABRIC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1))
#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1))
#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1))

bool dcm_mcu_misccfg_gic_sync_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP_GIC_RGU_SYNC_DCM) &
		MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_gic_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_gic_sync_dcm'" */
		reg_write(MP_GIC_RGU_SYNC_DCM,
			(reg_read(MP_GIC_RGU_SYNC_DCM) &
			~MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_gic_sync_dcm'" */
		reg_write(MP_GIC_RGU_SYNC_DCM,
			(reg_read(MP_GIC_RGU_SYNC_DCM) &
			~MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_GIC_SYNC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_L2_SHARED_DCM_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_L2_SHARED_DCM_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_L2_SHARED_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcu_misccfg_l2_shared_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(L2C_SRAM_CTRL) &
		MCU_MISCCFG_L2_SHARED_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_L2_SHARED_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_l2_shared_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_l2_shared_dcm'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCU_MISCCFG_L2_SHARED_DCM_REG0_MASK) |
			MCU_MISCCFG_L2_SHARED_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_l2_shared_dcm'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCU_MISCCFG_L2_SHARED_DCM_REG0_MASK) |
			MCU_MISCCFG_L2_SHARED_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcu_misccfg_mp0_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP0_PLL_DIVIDER_CFG) &
		MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp0_arm_pll_divider_dcm'" */
		reg_write(MP0_PLL_DIVIDER_CFG,
			(reg_read(MP0_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp0_arm_pll_divider_dcm'" */
		reg_write(MP0_PLL_DIVIDER_CFG,
			(reg_read(MP0_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK ((0x1 << 7))
#define MCU_MISCCFG_MP0_STALL_DCM_REG0_ON ((0x1 << 7))
#define MCU_MISCCFG_MP0_STALL_DCM_REG0_OFF ((0x0 << 7))
#if 0
static unsigned int mcu_misccfg_mp0_sync_dcm_stall_wr_del_sel_get(void)
{
	return (reg_read(SYNC_DCM_CLUSTER_CONFIG) >> 0) & 0x1f;
}
static void mcu_misccfg_mp0_sync_dcm_stall_wr_del_sel_set(unsigned int val)
{
	reg_write(SYNC_DCM_CLUSTER_CONFIG,
		(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		~(0x1f << 0)) |
		(val & 0x1f) << 0);
}
#endif

bool dcm_mcu_misccfg_mp0_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP0_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp0_stall_dcm'" */
#if 0
		mcu_misccfg_mp0_sync_dcm_stall_wr_del_sel_set(0x0);
#endif
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp0_stall_dcm'" */
#if 0
		mcu_misccfg_mp0_sync_dcm_stall_wr_del_sel_set(0x1);
#endif
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_STALL_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_MASK ((0x1 << 9))
#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_ON ((0x1 << 9))
#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_OFF ((0x0 << 9))
#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG_MASK ((0x1 << 10))
#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG1 ((0x1 << 10))
#define MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG0 ((0x1 << 10))
#if 0
static unsigned int mcu_misccfg_mp0_sync_dcm_div_sel_lo_get(void)
{
	return (reg_read(SYNC_DCM_CONFIG) >> 11) & 0xf;
}
#endif
#if 0
static unsigned int mcu_misccfg_mp0_sync_dcm_div_sel_hi_get(void)
{
	return (reg_read(SYNC_DCM_CONFIG) >> 25) & 0x7;
}
#endif
#if 0
static void mcu_misccfg_mp0_sync_dcm_div_sel_lo_set(unsigned int val)
{
	reg_write(SYNC_DCM_CONFIG,
		(reg_read(SYNC_DCM_CONFIG) &
		~(0xf << 11)) |
		(val & 0xf) << 11);
}
static void mcu_misccfg_mp0_sync_dcm_div_sel_hi_set(unsigned int val)
{
	reg_write(SYNC_DCM_CONFIG,
		(reg_read(SYNC_DCM_CONFIG) &
		~(0x7 << 25)) |
		(val & 0x7) << 25);
}
#endif

bool dcm_mcu_misccfg_mp0_sync_dcm_enable_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CONFIG) &
		MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_sync_dcm_enable(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp0_sync_dcm_enable'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG0);
		mcu_misccfg_mp0_sync_dcm_div_sel_lo_set(0x0);
		mcu_misccfg_mp0_sync_dcm_div_sel_hi_set(0x0);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_ON);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG1);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp0_sync_dcm_enable'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG0);
		mcu_misccfg_mp0_sync_dcm_div_sel_lo_set(0x1);
		mcu_misccfg_mp0_sync_dcm_div_sel_hi_set(0x0);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_OFF);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_ENABLE_REG0_TOG1);
#endif
	}
}

#define MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON ((0x1 << 11) | \
			(0x1 << 24) | \
			(0x1 << 25))
#define MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_OFF ((0x0 << 11) | \
			(0x0 << 24) | \
			(0x0 << 25))

bool dcm_mcu_misccfg_mp1_arm_pll_divider_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MP1_PLL_DIVIDER_CFG) &
		MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_arm_pll_divider_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp1_arm_pll_divider_dcm'" */
		reg_write(MP1_PLL_DIVIDER_CFG,
			(reg_read(MP1_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp1_arm_pll_divider_dcm'" */
		reg_write(MP1_PLL_DIVIDER_CFG,
			(reg_read(MP1_PLL_DIVIDER_CFG) &
			~MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_ARM_PLL_DIVIDER_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK ((0x1 << 15))
#define MCU_MISCCFG_MP1_STALL_DCM_REG0_ON ((0x1 << 15))
#define MCU_MISCCFG_MP1_STALL_DCM_REG0_OFF ((0x0 << 15))
#if 0
static unsigned int mcu_misccfg_mp1_sync_dcm_stall_wr_del_sel_get(void)
{
	return (reg_read(SYNC_DCM_CLUSTER_CONFIG) >> 8) & 0x1f;
}
static void mcu_misccfg_mp1_sync_dcm_stall_wr_del_sel_set(unsigned int val)
{
	reg_write(SYNC_DCM_CLUSTER_CONFIG,
		(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		~(0x1f << 8)) |
		(val & 0x1f) << 8);
}
#endif

bool dcm_mcu_misccfg_mp1_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP1_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp1_stall_dcm'" */
#if 0
		mcu_misccfg_mp1_sync_dcm_stall_wr_del_sel_set(0x0);
#endif
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp1_stall_dcm'" */
#if 0
		mcu_misccfg_mp1_sync_dcm_stall_wr_del_sel_set(0x1);
#endif
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_STALL_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK ((0x1 << 16))
#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_ON ((0x1 << 16))
#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_OFF ((0x0 << 16))
#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG_MASK ((0x1 << 17))
#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG1 ((0x1 << 17))
#define MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG0 ((0x1 << 17))
#if 0
static unsigned int mcu_misccfg_mp1_sync_dcm_div_sel_get(void)
{
	return (reg_read(SYNC_DCM_CONFIG) >> 18) & 0x7f;
}
static void mcu_misccfg_mp1_sync_dcm_div_sel_set(unsigned int val)
{
	reg_write(SYNC_DCM_CONFIG,
		(reg_read(SYNC_DCM_CONFIG) &
		~(0x7f << 18)) |
		(val & 0x7f) << 18);
}
#endif

bool dcm_mcu_misccfg_mp1_sync_dcm_enable_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CONFIG) &
		MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_sync_dcm_enable(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp1_sync_dcm_enable'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG0);
		mcu_misccfg_mp1_sync_dcm_div_sel_set(0x0);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_ON);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG1);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp1_sync_dcm_enable'" */
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG0);
		mcu_misccfg_mp1_sync_dcm_div_sel_set(0x1);
#endif
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_OFF);
#if 0
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_ENABLE_REG0_TOG1);
#endif
	}
}

#define MCU_MISCCFG_MP_STALL_DCM_REG0_MASK ((0xf << 24))
#define MCU_MISCCFG_MP_STALL_DCM_REG0_ON ((0x6 << 24))
#define MCU_MISCCFG_MP_STALL_DCM_REG0_OFF ((0xf << 24))

bool dcm_mcu_misccfg_mp_stall_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		MCU_MISCCFG_MP_STALL_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MP_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP_STALL_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MCU_MISC_DCM_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1))
#define MCU_MISCCFG_MCU_MISC_DCM_REG0_ON ((0x1 << 0) | \
			(0x1 << 1))
#define MCU_MISCCFG_MCU_MISC_DCM_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1))

bool dcm_mcu_misccfg_mcu_misc_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MCU_MISC_DCM_CTRL) &
		MCU_MISCCFG_MCU_MISC_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISCCFG_MCU_MISC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mcu_misc_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mcu_misc_dcm'" */
		reg_write(MCU_MISC_DCM_CTRL,
			(reg_read(MCU_MISC_DCM_CTRL) &
			~MCU_MISCCFG_MCU_MISC_DCM_REG0_MASK) |
			MCU_MISCCFG_MCU_MISC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mcu_misc_dcm'" */
		reg_write(MCU_MISC_DCM_CTRL,
			(reg_read(MCU_MISC_DCM_CTRL) &
			~MCU_MISCCFG_MCU_MISC_DCM_REG0_MASK) |
			MCU_MISCCFG_MCU_MISC_DCM_REG0_OFF);
	}
}

#define MCU_MISC1CFG_MCSIA_DCM_REG0_MASK ((0xffff << 16))
#define MCU_MISC1CFG_MCSIA_DCM_REG0_ON ((0x0 << 16))
#define MCU_MISC1CFG_MCSIA_DCM_REG0_OFF ((0xffff << 16))

bool dcm_mcu_misc1cfg_mcsia_dcm_is_on(void)
{
	bool ret = true;

	ret &= ((reg_read(MCSIA_DCM_EN) &
		MCU_MISC1CFG_MCSIA_DCM_REG0_MASK) ==
		(unsigned int) MCU_MISC1CFG_MCSIA_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misc1cfg_mcsia_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misc1cfg_mcsia_dcm'" */
		reg_write(MCSIA_DCM_EN,
			(reg_read(MCSIA_DCM_EN) &
			~MCU_MISC1CFG_MCSIA_DCM_REG0_MASK) |
			MCU_MISC1CFG_MCSIA_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misc1cfg_mcsia_dcm'" */
		reg_write(MCSIA_DCM_EN,
			(reg_read(MCSIA_DCM_EN) &
			~MCU_MISC1CFG_MCSIA_DCM_REG0_MASK) |
			MCU_MISC1CFG_MCSIA_DCM_REG0_OFF);
	}
}

