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

#ifndef _WMT_PLAT_H_
#define _WMT_PLAT_H_
#include "osal_typedef.h"
#include "stp_exp.h"
#include <mtk_wcn_cmb_stub.h>
#include "mtk_wcn_cmb_hw.h"

/* #include "mtk_wcn_consys_hw.h" */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#if 1				/* moved from wmt_exp.h */
#ifndef DFT_TAG
#define DFT_TAG         "[WMT-DFT]"
#endif

#define WMT_PLAT_LOG_LOUD                 4
#define WMT_PLAT_LOG_DBG                  3
#define WMT_PLAT_LOG_INFO                 2
#define WMT_PLAT_LOG_WARN                 1
#define WMT_PLAT_LOG_ERR                  0

extern UINT32 wmtPlatLogLvl;

#define WMT_PLAT_LOUD_FUNC(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_LOUD) \
		pr_warn(DFT_TAG "[L]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_INFO_FUNC(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_INFO) \
		pr_warn(DFT_TAG "[I]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_WARN_FUNC(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_WARN) \
		pr_warn(DFT_TAG "[W]%s:"  fmt, __func__, ##arg); \
} while (0)
#define WMT_PLAT_ERR_FUNC(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_ERR) \
		pr_err(DFT_TAG "[E]%s(%d):"  fmt, __func__, __LINE__, ##arg); \
} while (0)
#define WMT_PLAT_DBG_FUNC(fmt, arg...) \
do { \
	if (wmtPlatLogLvl >= WMT_PLAT_LOG_DBG) \
		pr_warn(DFT_TAG "[D]%s:"  fmt, __func__, ##arg); \
} while (0)

#endif

#define CFG_WMT_PS_SUPPORT 1	/* moved from wmt_exp.h */

#define CFG_WMT_DUMP_INT_STATUS 0
#define CONSYS_ENALBE_SET_JTAG 1

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*								  M A C R O S
********************************************************************************
*/
#if MTK_WCN_CMB_FOR_SDIO_1V_AUTOK
#define WMT_FOR_SDIO_1V_AUTOK 1
#else
#define WMT_FOR_SDIO_1V_AUTOK 0
#endif

#if (defined(MT6630) || defined(MT6632))
#define CONSYS_WMT_REG_SUSPEND_CB_ENABLE 1
#else
#define CONSYS_WMT_REG_SUSPEND_CB_ENABLE 0
#endif

#if defined(MERGE_INTERFACE_SUPPORT) && (defined(MT6628) || defined(MT6630) || defined(MT6632))
#define MTK_WCN_CMB_MERGE_INTERFACE_SUPPORT 1
#else
#define MTK_WCN_CMB_MERGE_INTERFACE_SUPPORT 0
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
enum ENUM_PIN_ID {
	PIN_LDO = 0,
	PIN_PMU = 1,
	PIN_RTC = 2,
	PIN_RST = 3,
	PIN_BGF_EINT = 4,
	PIN_WIFI_EINT = 5,
	PIN_ALL_EINT = 6,
	PIN_UART_GRP = 7,
	PIN_PCM_GRP = 8,
	PIN_I2S_GRP = 9,
	PIN_SDIO_GRP = 10,
	PIN_GPS_SYNC = 11,
	PIN_GPS_LNA = 12,
	PIN_UART_RX = 13,
#if CFG_WMT_LTE_COEX_HANDLING
	PIN_TDM_REQ = 14,
#endif
	PIN_ID_MAX
};
#if 0
enum ENUM_PIN_ID {
	PIN_BGF_EINT = 0,
	PIN_I2S_GRP = 1,
	PIN_GPS_SYNC = 2,
	PIN_GPS_LNA = 3,
#if CFG_WMT_LTE_COEX_HANDLING
	PIN_TDM_REQ = 4,
#endif
	PIN_ID_MAX
};
#endif

enum ENUM_FUNC_STATE {
	FUNC_ON = 0,
	FUNC_OFF = 1,
	FUNC_RST = 2,
	FUNC_STAT = 3,
	FUNC_CTRL_MAX,
};

enum ENUM_PIN_STATE {
	PIN_STA_INIT = 0,
	PIN_STA_OUT_L = 1,
	PIN_STA_OUT_H = 2,
	PIN_STA_IN_L = 3,
	PIN_STA_MUX = 4,
	PIN_STA_EINT_EN = 5,
	PIN_STA_EINT_DIS = 6,
	PIN_STA_DEINIT = 7,
	PIN_STA_SHOW = 8,
	PIN_STA_IN_PU = 9,
	PIN_STA_IN_NP = 10,
	PIN_STA_IN_H = 11,
	PIN_STA_MAX
};

enum CMB_IF_TYPE {
	CMB_IF_UART = 0,
	CMB_IF_WIFI_SDIO = 1,
	CMB_IF_BGF_SDIO = 2,
	CMB_IF_BGWF_SDIO = 3,
	CMB_IF_TYPE_MAX
};

typedef INT32(*fp_set_pin) (enum ENUM_PIN_STATE);

enum ENUM_WL_OP {
	WL_OP_GET = 0,
	WL_OP_PUT = 1,
	WL_OP_MAX
};

enum ENUM_PALDO_TYPE {
	BT_PALDO = 0,
	WIFI_PALDO = 1,
	FM_PALDO = 2,
	GPS_PALDO = 3,
	PMIC_CHIPID_PALDO = 4,
	WIFI_5G_PALDO = 5,
	PALDO_TYPE_MAX
};

enum ENUM_PALDO_OP {
	PALDO_OFF = 0,
	PALDO_ON = 1,
	PALDO_OP_MAX
};

enum ENUM_HOST_DUMP_STATE {
	STP_HOST_DUMP_NOT_START = 0,
	STP_HOST_DUMP_GET = 1,
	STP_HOST_DUMP_GET_DONE = 2,
	STP_HOST_DUMP_END = 3,
	STP_HOST_DUMP_MAX
};

enum ENUM_FORCE_TRG_ASSERT_T {
	STP_FORCE_TRG_ASSERT_EMI = 0,
	STP_FORCE_TRG_ASSERT_DEBUG_PIN = 1,
	STP_FORCE_TRG_ASSERT_MAX = 2
};

enum ENUM_CHIP_DUMP_STATE {
	STP_CHIP_DUMP_NOT_START = 0,
	STP_CHIP_DUMP_PUT = 1,
	STP_CHIP_DUMP_PUT_DONE = 2,
	STP_CHIP_DUMP_END = 3,
	STP_CHIP_DUMP_MAX
};

#define CONSYS_BUS_CLK_STATUS_OFFSET	0x00000100
#define CONSYS_CPU_CLK_STATUS_OFFSET	0x0000010c
#define CONSYS_DBG_CR1_OFFSET		0x00000408
#define CONSYS_DBG_CR2_OFFSET		0x0000040c
enum ENUM_CONNSYS_DEBUG_CR {
	CONNSYS_CPU_CLK = 0,
	CONNSYS_BUS_CLK = 1,
	CONNSYS_DEBUG_CR1 = 2,
	CONNSYS_DEBUG_CR2 = 3,
	CONNSYS_CR_MAX
};

struct EMI_CTRL_STATE_OFFSET {
	UINT32 emi_apmem_ctrl_state;
	UINT32 emi_apmem_ctrl_host_sync_state;
	UINT32 emi_apmem_ctrl_host_sync_num;
	UINT32 emi_apmem_ctrl_chip_sync_state;
	UINT32 emi_apmem_ctrl_chip_sync_num;
	UINT32 emi_apmem_ctrl_chip_sync_addr;
	UINT32 emi_apmem_ctrl_chip_sync_len;
	UINT32 emi_apmem_ctrl_chip_print_buff_start;
	UINT32 emi_apmem_ctrl_chip_print_buff_len;
	UINT32 emi_apmem_ctrl_chip_print_buff_idx;
	UINT32 emi_apmem_ctrl_chip_int_status;
	UINT32 emi_apmem_ctrl_chip_paded_dump_end;
	UINT32 emi_apmem_ctrl_host_outband_assert_w1;
	UINT32 emi_apmem_ctrl_chip_page_dump_num;
};

struct BGF_IRQ_BALANCE {
	UINT32 counter;
	unsigned long flags;
	spinlock_t lock;
};

struct CONSYS_EMI_ADDR_INFO {
	UINT32 emi_phy_addr;
	UINT32 paged_trace_off;
	UINT32 paged_dump_off;
	UINT32 full_dump_off;
	struct EMI_CTRL_STATE_OFFSET *p_ecso;
};

struct GPIO_TDM_REQ_INFO {
	UINT32 ant_sel_index;
	UINT32 gpio_number;
	UINT32 cr_address;
};

typedef VOID(*irq_cb) (VOID);
typedef INT32(*device_audio_if_cb) (enum CMB_STUB_AIF_X aif, MTK_WCN_BOOL share);
typedef VOID(*func_ctrl_cb) (UINT32 on, UINT32 type);
typedef long (*thermal_query_ctrl_cb) (VOID);
typedef INT32(*deep_idle_ctrl_cb) (UINT32);

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
extern UINT32 gWmtDbgLvl;
extern struct device *wmt_dev;
#ifdef CFG_WMT_READ_EFUSE_VCN33
extern INT32 wmt_set_pmic_voltage(UINT32 level);
#endif
/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

INT32 wmt_plat_init(struct PWR_SEQ_TIME *pPwrSeqTime, UINT32 co_clock_type);
INT32 wmt_plat_deinit(VOID);
INT32 wmt_plat_merge_if_flag_get(VOID);
INT32 wmt_plat_set_comm_if_type(enum ENUM_STP_TX_IF_TYPE type);
INT32 wmt_plat_merge_if_flag_ctrl(UINT32 enagle);
enum ENUM_STP_TX_IF_TYPE wmt_plat_get_comm_if_type(VOID);

INT32 wmt_plat_pwr_ctrl(enum ENUM_FUNC_STATE state);

INT32 wmt_plat_gpio_ctrl(enum ENUM_PIN_ID id, enum ENUM_PIN_STATE state);

INT32 wmt_plat_eirq_ctrl(enum ENUM_PIN_ID id, enum ENUM_PIN_STATE state);

INT32 wmt_plat_wake_lock_ctrl(enum ENUM_WL_OP opId);
INT32 wmt_plat_sdio_ctrl(UINT32 sdioPortNum, enum ENUM_FUNC_STATE on);

INT32 wmt_plat_audio_ctrl(enum CMB_STUB_AIF_X state, enum CMB_STUB_AIF_CTRL ctrl);
VOID wmt_lib_plat_irq_cb_reg(irq_cb bgf_irq_cb);
VOID wmt_lib_plat_aif_cb_reg(device_audio_if_cb aif_ctrl_cb);

VOID wmt_plat_irq_cb_reg(irq_cb bgf_irq_cb);
VOID wmt_plat_aif_cb_reg(device_audio_if_cb aif_ctrl_cb);
VOID wmt_plat_func_ctrl_cb_reg(func_ctrl_cb subsys_func_ctrl);
VOID wmt_plat_thermal_ctrl_cb_reg(thermal_query_ctrl_cb thermal_query_ctrl);
VOID wmt_plat_deep_idle_ctrl_cb_reg(deep_idle_ctrl_cb deep_idle_ctrl);

INT32 wmt_plat_soc_paldo_ctrl(enum ENUM_PALDO_TYPE ePt, enum ENUM_PALDO_OP ePo);
UINT8 *wmt_plat_get_emi_virt_add(UINT32 offset);
#if CONSYS_ENALBE_SET_JTAG
UINT32 wmt_plat_jtag_flag_ctrl(UINT32 en);
#endif
#if CFG_WMT_DUMP_INT_STATUS
VOID wmt_plat_BGF_irq_dump_status(VOID);
MTK_WCN_BOOL wmt_plat_dump_BGF_irq_status(VOID);
#endif
struct CONSYS_EMI_ADDR_INFO *wmt_plat_get_emi_phy_add(VOID);
UINT32 wmt_plat_read_cpupcr(VOID);
UINT32 wmt_plat_read_dmaregs(UINT32);
INT32 wmt_plat_set_host_dump_state(enum ENUM_HOST_DUMP_STATE state);
UINT32 wmt_plat_force_trigger_assert(enum ENUM_FORCE_TRG_ASSERT_T type);
INT32 wmt_plat_update_host_sync_num(VOID);
INT32 wmt_plat_get_dump_info(UINT32 offset);
UINT32 wmt_plat_get_soc_chipid(VOID);
UINT32 wmt_plat_soc_co_clock_flag_get(VOID);
INT32 wmt_plat_set_dbg_mode(UINT32 flag);
INT32 wmt_plat_set_dynamic_dumpmem(PUINT32 buf);
#if CFG_WMT_LTE_COEX_HANDLING
INT32 wmt_plat_get_tdm_antsel_index(VOID);
#endif
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _WMT_PLAT_H_ */
