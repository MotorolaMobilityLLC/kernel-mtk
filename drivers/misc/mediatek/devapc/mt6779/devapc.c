/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include <mt-plat/mtk_secure_api.h>
#include <mt-plat/devapc_public.h>

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

/* CCF */
#include <linux/clk.h>

#include "mtk_io.h"
#include "sync_write.h"
#include "devapc.h"
#include "busid_to_mast.h"

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
#include <mt-plat/aee.h>
#endif

/* 0 for early porting */
#define DEVAPC_TURN_ON         1
#define DEVAPC_USE_CCF         1


/* bypass clock! */
#if DEVAPC_USE_CCF
static struct clk *dapc_infra_clk;
#endif

static struct cdev *g_devapc_ctrl;
static unsigned int devapc_infra_irq;
static void __iomem *devapc_pd_infra_base;
static void __iomem *devapc_ao_infra_base;

unsigned int dbg1;
unsigned int master_id;
unsigned int domain_id;

#ifdef DBG_ENABLE
static unsigned int enable_dynamic_one_core_violation_debug;
#endif

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
unsigned int devapc_vio_current_aee_trigger_times;
#endif

#if DEVAPC_TURN_ON
static struct DEVICE_INFO devapc_infra_devices[] = {
/* slave type,       config_idx, device name                enable_vio_irq */

 /* 0 */
{E_DAPC_INFRA_PERI_SLAVE, 0,   "INFRA_AO_TOPCKGEN",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 1,   "INFRA_AO_INFRASYS_CONFIG_REGS",         true},
{E_DAPC_INFRA_PERI_SLAVE, 2,   "INFRA_AO_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 3,   "INFRA_AO_ PERICFG",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 4,   "INFRA_AO_EFUSE_AO_DEBUG",               true},
{E_DAPC_INFRA_PERI_SLAVE, 5,   "INFRA_AO_GPIO",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 6,   "INFRA_AO_SLEEP_CONTROLLER",             true},
{E_DAPC_INFRA_PERI_SLAVE, 7,   "INFRA_AO_TOPRGU",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 8,   "INFRA_AO_APXGPT",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 9,   "INFRA_AO_RESERVE",                      true},

 /* 10 */
{E_DAPC_INFRA_PERI_SLAVE, 10,  "INFRA_AO_SEJ",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 11,  "INFRA_AO_AP_CIRQ_EINT",                 true},
{E_DAPC_INFRA_PERI_SLAVE, 12,  "INFRA_AO_APMIXEDSYS",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 13,  "INFRA_AO_PMIC_WRAP",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 14,  "INFRA_AO_DEVICE_APC_AO_MM",             true},
{E_DAPC_INFRA_PERI_SLAVE, 15,  "INFRA_AO_SLEEP_CONTROLLER_MD",          true},
{E_DAPC_INFRA_PERI_SLAVE, 16,  "INFRA_AO_KEYPAD",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 17,  "INFRA_AO_TOP_MISC",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 18,  "INFRA_AO_ DVFS_CTRL_PROC",              true},
{E_DAPC_INFRA_PERI_SLAVE, 19,  "INFRA_AO_MBIST_AO_REG",                 true},

 /* 20 */
{E_DAPC_INFRA_PERI_SLAVE, 20,  "INFRA_AO_CLDMA_AO_AP",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 21,  "INFRA_AO_DEVICE_MPU",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 22,  "INFRA_AO_AES_TOP_0",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 23,  "INFRA_AO_SYS_TIMER",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 24,  "INFRA_AO_MDEM_TEMP_SHARE",              true},
{E_DAPC_INFRA_PERI_SLAVE, 25,  "INFRA_AO_DEVICE_APC_AO_MD",             true},
{E_DAPC_INFRA_PERI_SLAVE, 26,  "INFRA_AO_SECURITY_AO",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 27,  "INFRA_AO_TOPCKGEN_REG",                 true},
{E_DAPC_INFRA_PERI_SLAVE, 28,  "INFRA_AO_DEVICE_APC_AO_INFRA_PERI",     true},
{E_DAPC_INFRA_PERI_SLAVE, 29,  "INFRA_AO_SLEEP_SRAM",                   true},

 /* 30 */
{E_DAPC_INFRA_PERI_SLAVE, 30,  "INFRA_AO_SLEEP_SRAM",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 31,  "INFRA_AO_SLEEP_SRAM",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 32,  "INFRA_AO_SLEEP_SRAM",                   true},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, 37,  "INFRASYS_SYS_CIRQ",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 38,  "INFRASYS_MM_IOMMU",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 39,  "INFRASYS_EFUSE_PDN_DEBUG",              true},

 /* 40 */
{E_DAPC_INFRA_PERI_SLAVE, 40,  "INFRASYS_DEVICE_APC",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 41,  "INFRASYS_DBG_TRACKER",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 42,  "INFRASYS_CCIF0_AP",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 43,  "INFRASYS_CCIF0_MD",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 44,  "INFRASYS_CCIF1_AP",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 45,  "INFRASYS_CCIF1_MD",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 46,  "INFRASYS_MBIST",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 47,  "INFRASYS_INFRA_PDN_REGISTER",           true},
{E_DAPC_INFRA_PERI_SLAVE, 48,  "INFRASYS_TRNG",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 49,  "INFRASYS_DX_CC",                        true},

 /* 50 */
{E_DAPC_INFRA_PERI_SLAVE, 50,  "INFRASYS_MD_CCIF_MD1",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 51,  "INFRASYS_CQ_DMA",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 52,  "INFRASYS_MD_CCIF_MD2",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 53,  "INFRASYS_SRAMROM",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 54,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 55,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 56,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 57,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 58,  "INFRASYS_EMI",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 59,  "INFRASYS_DEVICE_MPU",                   true},

 /* 60 */
{E_DAPC_INFRA_PERI_SLAVE, 60,  "INFRASYS_CLDMA_PDN",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 61,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 62,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 63,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 64,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},

 /* 70 */
{E_DAPC_INFRA_PERI_SLAVE, 65,  "INFRASYS_RESERVE",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 66,  "INFRASYS_EMI_MPU",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 67,  "INFRASYS_DVFS_PROC",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 68,  "INFRASYS_GCE",                          true},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, -1,  "INFRASYS_RESERVE",                      false},
{E_DAPC_INFRA_PERI_SLAVE, 33,  "INFRASYS_DPMAIF",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 34,  "INFRASYS_DPMAIF",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 35,  "INFRASYS_DPMAIF",                       true},

 /* 80 */
{E_DAPC_INFRA_PERI_SLAVE, 35,  "INFRASYS_DPMAIF",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 70,  "INFRASYS_DRAMC_CH1_TOP0",               true},
{E_DAPC_INFRA_PERI_SLAVE, 71,  "INFRASYS_DRAMC_CH1_TOP1",               true},
{E_DAPC_INFRA_PERI_SLAVE, 72,  "INFRASYS_DRAMC_CH1_TOP2",               true},
{E_DAPC_INFRA_PERI_SLAVE, 73,  "INFRASYS_DRAMC_CH1_TOP3",               true},
{E_DAPC_INFRA_PERI_SLAVE, 74,  "INFRASYS_DRAMC_CH1_TOP4",               true},
{E_DAPC_INFRA_PERI_SLAVE, 75,  "INFRASYS_DRAMC_CH1_TOP5",               true},
{E_DAPC_INFRA_PERI_SLAVE, 76,  "INFRASYS_DRAMC_CH1_TOP6",               true},
{E_DAPC_INFRA_PERI_SLAVE, 77,  "INFRASYS_CCIF2_AP",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 78,  "INFRASYS_CCIF2_MD",                     true},

 /* 90 */
{E_DAPC_INFRA_PERI_SLAVE, 79,  "INFRASYS_CCIF3_AP",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 80,  "INFRASYS_CCIF3_MD",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 81,  "INFRASYS_DRAMC_CH0_TOP0",               true},
{E_DAPC_INFRA_PERI_SLAVE, 82,  "INFRASYS_DRAMC_CH0_TOP1",               true},
{E_DAPC_INFRA_PERI_SLAVE, 83,  "INFRASYS_DRAMC_CH0_TOP2",               true},
{E_DAPC_INFRA_PERI_SLAVE, 84,  "INFRASYS_DRAMC_CH0_TOP3",               true},
{E_DAPC_INFRA_PERI_SLAVE, 85,  "INFRASYS_DRAMC_CH0_TOP4",               true},
{E_DAPC_INFRA_PERI_SLAVE, 86,  "INFRASYS_DRAMC_CH0_TOP5",               true},
{E_DAPC_INFRA_PERI_SLAVE, 87,  "INFRASYS_DRAMC_CH0_TOP6",               true},
{E_DAPC_INFRA_PERI_SLAVE, 88,  "INFRASYS_CCIF4_AP",                     true},

 /* 100 */
{E_DAPC_INFRA_PERI_SLAVE, 89,  "INFRASYS_CCIF4_MD",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 90,  "INFRA_TOPAXI_BUS_TRACE",                true},
{E_DAPC_INFRA_PERI_SLAVE, 91,  "vpu_iommu/mm_iommu",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 92,  "vpu_iommu/mm_iommu",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 93,  "vpu_iommu/mm_iommu",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 94,  "vpu_iommu/mm_iommu",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 95,  "vpu_iommu/mm_iommu",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 96,  "sub_common2x1",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 97,  "sub_common2x1",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 98,  "sub_common2x1",                         true},

 /* 110 */
{E_DAPC_INFRA_PERI_SLAVE, 110, "INFRA_AO_SSPM Partition 1",             true},
{E_DAPC_INFRA_PERI_SLAVE, 111, "INFRA_AO_SSPM Partition 2",             true},
{E_DAPC_INFRA_PERI_SLAVE, 112, "INFRA_AO_SSPM Partition 3",             true},
{E_DAPC_INFRA_PERI_SLAVE, 113, "INFRA_AO_SSPM Partition 4",             true},
{E_DAPC_INFRA_PERI_SLAVE, 114, "INFRA_AO_SSPM Partition 5",             true},
{E_DAPC_INFRA_PERI_SLAVE, 115, "INFRA_AO_SSPM Partition 6",             true},
{E_DAPC_INFRA_PERI_SLAVE, 116, "INFRA_AO_SSPM Partition 7",             true},
{E_DAPC_INFRA_PERI_SLAVE, 117, "INFRA_AO_SSPM Partition 8",             true},
{E_DAPC_INFRA_PERI_SLAVE, 118, "INFRA_AO_SSPM Partition 9",             true},
{E_DAPC_INFRA_PERI_SLAVE, 119, "INFRA_AO_SSPM Partition 10",            true},

 /* 120 */
{E_DAPC_INFRA_PERI_SLAVE, 120, "INFRA_AO_SSPM Partition 11",            true},
{E_DAPC_INFRA_PERI_SLAVE, 121, "INFRA_AO_SCP",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 122, "INFRA_AO_HIFI3",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 123, "INFRA_AO_MCUCFG",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 124, "INFRASYS_DBUGSYS",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 125, "PERISYS_APDMA",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 126, "PERISYS_AUXADC",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 127, "PERISYS_UART0",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 128, "PERISYS_UART1",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 129, "PERISYS_UART2",                         true},

 /* 130 */
{E_DAPC_INFRA_PERI_SLAVE, 130, "PERISYS_UART3",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 131, "PERISYS_PWM",                           true},
{E_DAPC_INFRA_PERI_SLAVE, 132, "PERISYS_I2C0",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 133, "PERISYS_I2C1",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 134, "PERISYS_I2C2",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 135, "PERISYS_SPI0",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 136, "PERISYS_PTP",                           true},
{E_DAPC_INFRA_PERI_SLAVE, 137, "PERISYS_BTIF",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 138, "PERISYS_IRTX",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 139, "PERISYS_DISP_PWM",                      true},

 /* 140 */
{E_DAPC_INFRA_PERI_SLAVE, 140, "PERISYS_I2C3",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 141, "PERISYS_SPI1",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 142, "PERISYS_I2C4",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 143, "PERISYS_SPI2",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 144, "PERISYS_SPI3",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 145, "PERISYS_I2C1_IMM",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 146, "PERISYS_I2C2_IMM",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 147, "PERISYS_I2C5",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 148, "PERISYS_I2C5_IMM",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 149, "PERISYS_SPI4",                          true},

 /* 150 */
{E_DAPC_INFRA_PERI_SLAVE, 150, "PERISYS_SPI5",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 151, "PERISYS_I2C7",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 152, "PERISYS_I2C8",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 153, "PERISYS_BUS_TRACE",                     true},
{E_DAPC_INFRA_PERI_SLAVE, 154, "PERISYS_SPI6",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 155, "PERISYS_SPI7",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 156, "PERISYS_USB",                           true},
{E_DAPC_INFRA_PERI_SLAVE, 157, "PERISYS_USB_2.0_SUB",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 158, "PERISYS_AUDIO",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 159, "PERISYS_MSDC0",                         true},

 /* 160 */
{E_DAPC_INFRA_PERI_SLAVE, 160, "PERISYS_MSDC1",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 161, "PERISYS_MSDC2",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 162, "PERISYS_MSDC3",                         true},
{E_DAPC_INFRA_PERI_SLAVE, 163, "PERISYS_UFS",                           true},
{E_DAPC_INFRA_PERI_SLAVE, 164, "PERISUS_USB3.0_SIF",                    true},
{E_DAPC_INFRA_PERI_SLAVE, 165, "PERISUS_USB3.0_SIF2",                   true},
{E_DAPC_INFRA_PERI_SLAVE, 166, "EAST_RESERVE_0",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 167, "EAST_ RESERVE_1",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 168, "EAST_ RESERVE_2",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 169, "EAST_ RESERVE_3",                       true},

 /* 170 */
{E_DAPC_INFRA_PERI_SLAVE, 170, "EAST_ RESERVE_4",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 171, "EAST_IO_CFG_RT",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 172, "EAST_ RESERVE_6",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 173, "EAST_ RESERVE_7",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 174, "EAST_CSI0_TOP_AO",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 175, "EAST_CSI1_TOP_AO",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 176, "EAST_ RESERVE_A",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 177, "EAST_ RESERVE_B",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 178, "EAST_RESERVE_C",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 179, "EAST_RESERVE_D",                        true},

 /* 180 */
{E_DAPC_INFRA_PERI_SLAVE, 180, "EAST_RESERVE_E",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 181, "EAST_RESERVE_F",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 182, "SOUTH_RESERVE_0",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 183, "SOUTH_RESERVE_1",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 184, "SOUTH_IO_CFG_RM",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 185, "SOUTH_IO_CFG_RB",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 186, "SOUTH_EFUSE",                           true},
{E_DAPC_INFRA_PERI_SLAVE, 187, "SOUTH_ RESERVE_5",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 188, "SOUTH_ RESERVE_6",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 189, "SOUTH_ RESERVE_7",                      true},

 /* 190 */
{E_DAPC_INFRA_PERI_SLAVE, 190, "SOUTH_ RESERVE_8",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 191, "SOUTH_ RESERVE_9",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 192, "SOUTH_ RESERVE_A",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 193, "SOUTH_ RESERVE_B",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 194, "SOUTH_RESERVE_C",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 195, "SOUTH_RESERVE_D",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 196, "SOUTH_RESERVE_E",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 197, "SOUTH_RESERVE_F",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 198, "WEST_ RESERVE_0",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 199, "WEST_ msdc1_pad_macro",                 true},

 /* 200 */
{E_DAPC_INFRA_PERI_SLAVE, 200, "WEST_ RESERVE_2",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 201, "WEST_ RESERVE_3",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 202, "WEST_ RESERVE_4",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 203, "WEST_ MIPI_TX_CONFIG",                  true},
{E_DAPC_INFRA_PERI_SLAVE, 204, "WEST_ RESERVE_6",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 205, "WEST_ IO_CFG_LB",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 206, "WEST_ IO_CFG_LM",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 207, "WEST_ IO_CFG_BL",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 208, "WEST_ RESERVE_A",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 209, "WEST_ RESERVE_B",                       true},

 /* 210 */
{E_DAPC_INFRA_PERI_SLAVE, 210, "WEST_ RESERVE_C",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 211, "WEST_ RESERVE_D",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 212, "WEST_RESERVE_E",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 213, "WEST_RESERVE_F",                        true},
{E_DAPC_INFRA_PERI_SLAVE, 214, "NORTH_RESERVE_0",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 215, "NORTH_RESERVE_1",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 216, "NORTH_IO_CFG_LT",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 217, "NORTH_IO_CFG_TL",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 218, "NORTH_USB20 PHY",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 219, "NORTH_msdc0 pad macro",                 true},

 /* 220 */
{E_DAPC_INFRA_PERI_SLAVE, 220, "NORTH_ RESERVE_6",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 221, "NORTH_ RESERVE_7",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 222, "NORTH_ RESERVE_8",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 223, "NORTH_ RESERVE_9",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 224, "NORTH_ UFS_MPHY",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 225, "NORTH_ RESERVE_B",                      true},
{E_DAPC_INFRA_PERI_SLAVE, 226, "NORTH_RESERVE_C",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 227, "NORTH_RESERVE_D",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 228, "NORTH_RESERVE_E",                       true},
{E_DAPC_INFRA_PERI_SLAVE, 229, "NORTH_RESERVE_F",                       true},

 /* 230 */
{E_DAPC_INFRA_PERI_SLAVE, 230, "PERISYS_CONN",                          true},
{E_DAPC_INFRA_PERI_SLAVE, 231, "PERISYS_MD_VIOLATION",                  true},
{E_DAPC_MM_SLAVE,         0,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         1,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         2,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         3,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         4,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         5,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         6,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         7,   "Gpu",                                   true},

 /* 240 */
{E_DAPC_MM_SLAVE,         8,   "Gpu",                                   true},
{E_DAPC_MM_SLAVE,         9,   "DFD",                                   true},
{E_DAPC_MM_SLAVE,         10,  "G3D_CONFIG",                            true},
{E_DAPC_MM_SLAVE,         11,  "G3D_DVFS",                              true},
{E_DAPC_MM_SLAVE,         12,  "MFG_OTHERS",                            true},
{E_DAPC_MM_SLAVE,         13,  "MMSYS_CONFIG",                          true},
{E_DAPC_MM_SLAVE,         14,  "MDP_RDMA0",                             true},
{E_DAPC_MM_SLAVE,         15,  "MDP_RDMA1",                             true},
{E_DAPC_MM_SLAVE,         16,  "MDP_RSZ0",                              true},
{E_DAPC_MM_SLAVE,         17,  "MDP_RSZ1",                              true},

 /* 250 */
{E_DAPC_MM_SLAVE,         18,  "MDP_WROT0",                             true},
{E_DAPC_MM_SLAVE,         19,  "MDP_WDMA",                              true},
{E_DAPC_MM_SLAVE,         20,  "MDP_TDSHP",                             true},
{E_DAPC_MM_SLAVE,         21,  "DISP_OVL0",                             true},
{E_DAPC_MM_SLAVE,         22,  "DISP_OVL0_2L",                          true},
{E_DAPC_MM_SLAVE,         23,  "DISP_OVL1_2L",                          true},
{E_DAPC_MM_SLAVE,         24,  "DISP_RDMA0",                            true},
{E_DAPC_MM_SLAVE,         25,  "DISP_RDMA1",                            true},
{E_DAPC_MM_SLAVE,         26,  "DISP_WDMA0",                            true},
{E_DAPC_MM_SLAVE,         27,  "DISP_COLOR0",                           true},

 /* 260 */
{E_DAPC_MM_SLAVE,         28,  "DISP_CCORR0",                           true},
{E_DAPC_MM_SLAVE,         29,  "DISP_AAL0",                             true},
{E_DAPC_MM_SLAVE,         30,  "DISP_GAMMA0",                           true},
{E_DAPC_MM_SLAVE,         31,  "DISP_DITHER0",                          true},
{E_DAPC_MM_SLAVE,         32,  "DSI_SPLIT",                             true},
{E_DAPC_MM_SLAVE,         33,  "DSI0",                                  true},
{E_DAPC_MM_SLAVE,         34,  "DPI",                                   true},
{E_DAPC_MM_SLAVE,         35,  "MM_MUTEX",                              true},
{E_DAPC_MM_SLAVE,         36,  "SMI_LARB0",                             true},
{E_DAPC_MM_SLAVE,         37,  "SMI_LARB1",                             true},

 /* 270 */
{E_DAPC_MM_SLAVE,         38,  "SMI_COMMON",                            true},
{E_DAPC_MM_SLAVE,         39,  "DISP_RSZ",                              true},
{E_DAPC_MM_SLAVE,         40,  "MDP_AAL",                               true},
{E_DAPC_MM_SLAVE,         41,  "MDP_CCORR",                             true},
{E_DAPC_MM_SLAVE,         42,  "DBI",                                   true},
{E_DAPC_MM_SLAVE,         43,  "MMSYS_RESERVE",                         true},
{E_DAPC_MM_SLAVE,         44,  "MMSYS_RESERVE",                         true},
{E_DAPC_MM_SLAVE,         45,  "MDP_WROT1",                             true},
{E_DAPC_MM_SLAVE,         46,  "DISP_POSTMASK0",                        true},
{E_DAPC_MM_SLAVE,         47,  "MMSYS_OTHERS",                          true},

 /* 280 */
{E_DAPC_MM_SLAVE,         48,  "imgsys1_top",                           true},
{E_DAPC_MM_SLAVE,         49,  "dip_a0",                                true},
{E_DAPC_MM_SLAVE,         50,  "dip_a1",                                true},
{E_DAPC_MM_SLAVE,         51,  "dip_a2",                                true},
{E_DAPC_MM_SLAVE,         52,  "dip_a3",                                true},
{E_DAPC_MM_SLAVE,         53,  "dip_a4",                                true},
{E_DAPC_MM_SLAVE,         54,  "dip_a5",                                true},
{E_DAPC_MM_SLAVE,         55,  "dip_a6",                                true},
{E_DAPC_MM_SLAVE,         56,  "dip_a7",                                true},
{E_DAPC_MM_SLAVE,         57,  "dip_a8",                                true},

 /* 290 */
{E_DAPC_MM_SLAVE,         58,  "dip_a9",                                true},
{E_DAPC_MM_SLAVE,         59,  "dip_a10",                               true},
{E_DAPC_MM_SLAVE,         60,  "dip_a11",                               true},
{E_DAPC_MM_SLAVE,         61,  "IMGSYS_RESERVE",                        true},
{E_DAPC_MM_SLAVE,         62,  "smi_larb5",                             true},
{E_DAPC_MM_SLAVE,         63,  "smi_larb6",                             true},
{E_DAPC_MM_SLAVE,         64,  "IMGSYS_OTHERS",                         true},
{E_DAPC_MM_SLAVE,         65,  "VENCSYS_GLOBAL_CON",                    true},
{E_DAPC_MM_SLAVE,         66,  "VENCSYSSYS_SMI_LARB3",                  true},
{E_DAPC_MM_SLAVE,         67,  "VENCSYS_VENC",                          true},

 /* 300 */
{E_DAPC_MM_SLAVE,         68,  "VENCSYS_JPGENC",                        true},
{E_DAPC_MM_SLAVE,         69,  "VENC_RESERVE",                          true},
{E_DAPC_MM_SLAVE,         70,  "VENC_RESERVE",                          true},
{E_DAPC_MM_SLAVE,         71,  "VENC_RESERVE",                          true},
{E_DAPC_MM_SLAVE,         72,  "VENCSYS_MBIST_CTRL",                    true},
{E_DAPC_MM_SLAVE,         73,  "VENCSYS_OTHERS",                        true},
{E_DAPC_MM_SLAVE,         74,  "VDECSYS_GLOBAL_CON",                    true},
{E_DAPC_MM_SLAVE,         75,  "vdec_smi_larbx",                        true},
{E_DAPC_MM_SLAVE,         76,  "VDECSYS_FULL_TOP",                      true},
{E_DAPC_MM_SLAVE,         77,  "vdec_full_top_lat",                     true},

 /* 310 */
{E_DAPC_MM_SLAVE,         78,  "VDEC_RESERVE",                          true},
{E_DAPC_MM_SLAVE,         79,  "VDEC_RESERVE",                          true},
{E_DAPC_MM_SLAVE,         80,  "VDECSYS_OTHERS",                        true},
{E_DAPC_MM_SLAVE,         81,  "CAMSYS_CAMSYS_TOP",                     true},
{E_DAPC_MM_SLAVE,         82,  "CAMSYS_LARB9",                          true},
{E_DAPC_MM_SLAVE,         83,  "CAMSYS_LARB10",                         true},
{E_DAPC_MM_SLAVE,         84,  "CAMSYS_LARB11",                         true},
{E_DAPC_MM_SLAVE,         85,  "CAMSYS_seninf_a",                       true},
{E_DAPC_MM_SLAVE,         86,  "CAMSYS_seninf_b",                       true},
{E_DAPC_MM_SLAVE,         87,  "CAMSYS_seninf_c",                       true},

 /* 320 */
{E_DAPC_MM_SLAVE,         88,  "CAMSYS_seninf_d",                       true},
{E_DAPC_MM_SLAVE,         89,  "CAMSYS_seninf_e",                       true},
{E_DAPC_MM_SLAVE,         90,  "CAMSYS_seninf_f",                       true},
{E_DAPC_MM_SLAVE,         91,  "CAMSYS_seninf_g",                       true},
{E_DAPC_MM_SLAVE,         92,  "CAMSYS_seninf_h",                       true},
{E_DAPC_MM_SLAVE,         93,  "CAMSYS_smi_sub_common",                 true},
{E_DAPC_MM_SLAVE,         94,  "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         95,  "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         96,  "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         97,  "cam_raw_a_ip_group_0",                  true},

 /* 330 */
{E_DAPC_MM_SLAVE,         98,  "cam_raw_a_ip_group_1",                  true},
{E_DAPC_MM_SLAVE,         99,  "cam_raw_a_ip_group_2",                  true},
{E_DAPC_MM_SLAVE,         100, "cam_raw_a_ip_group_3",                  true},
{E_DAPC_MM_SLAVE,         101, "cam_raw_a_dma_0",                       true},
{E_DAPC_MM_SLAVE,         102, "cam_raw_a_dma_1",                       true},
{E_DAPC_MM_SLAVE,         103, "ltm_curve_a_0",                         true},
{E_DAPC_MM_SLAVE,         104, "ltm_curve_a_1",                         true},
{E_DAPC_MM_SLAVE,         105, "cam_raw_a_ip_group_0_inner",            true},
{E_DAPC_MM_SLAVE,         106, "cam_raw_a_ip_group_1_inner",            true},
{E_DAPC_MM_SLAVE,         107, "cam_raw_a_ip_group_2_inner",            true},

 /* 340 */
{E_DAPC_MM_SLAVE,         108, "cam_raw_a_ip_group_3_inner",            true},
{E_DAPC_MM_SLAVE,         109, "cam_raw_a_dma_0_inner",                 true},
{E_DAPC_MM_SLAVE,         110, "cam_raw_a_dma_1_inner",                 true},
{E_DAPC_MM_SLAVE,         111, "ltm_curve_a_0_inner",                   true},
{E_DAPC_MM_SLAVE,         112, "ltm_curve_a_1_inner",                   true},
{E_DAPC_MM_SLAVE,         113, "cam_raw_b_ip_group_0",                  true},
{E_DAPC_MM_SLAVE,         114, "cam_raw_b_ip_group_1",                  true},
{E_DAPC_MM_SLAVE,         115, "cam_raw_b_ip_group_2",                  true},
{E_DAPC_MM_SLAVE,         116, "cam_raw_b_ip_group_3",                  true},
{E_DAPC_MM_SLAVE,         117, "cam_raw_b_dma_0",                       true},

 /* 350 */
{E_DAPC_MM_SLAVE,         118, "cam_raw_b_dma_1",                       true},
{E_DAPC_MM_SLAVE,         119, "ltm_curve_b_0",                         true},
{E_DAPC_MM_SLAVE,         120, "ltm_curve_b_1",                         true},
{E_DAPC_MM_SLAVE,         121, "cam_raw_b_ip_group_0_inner",            true},
{E_DAPC_MM_SLAVE,         122, "cam_raw_b_ip_group_1_inner",            true},
{E_DAPC_MM_SLAVE,         123, "cam_raw_b_ip_group_2_inner",            true},
{E_DAPC_MM_SLAVE,         124, "cam_raw_b_ip_group_3_inner",            true},
{E_DAPC_MM_SLAVE,         125, "cam_raw_b_dma_0_inner",                 true},
{E_DAPC_MM_SLAVE,         126, "cam_raw_b_dma_1_inner",                 true},
{E_DAPC_MM_SLAVE,         127, "ltm_curve_b_0_inner",                   true},

 /* 360 */
{E_DAPC_MM_SLAVE,         128, "ltm_curve_b_1_inner",                   true},
{E_DAPC_MM_SLAVE,         129, "cam_raw_c_ip_group_0",                  true},
{E_DAPC_MM_SLAVE,         130, "cam_raw_c_ip_group_1",                  true},
{E_DAPC_MM_SLAVE,         131, "cam_raw_c_ip_group_2",                  true},
{E_DAPC_MM_SLAVE,         132, "cam_raw_c_ip_group_3",                  true},
{E_DAPC_MM_SLAVE,         133, "cam_raw_c_dma_0",                       true},
{E_DAPC_MM_SLAVE,         134, "cam_raw_c_dma_1",                       true},
{E_DAPC_MM_SLAVE,         135, "ltm_curve_c_0",                         true},
{E_DAPC_MM_SLAVE,         136, "ltm_curve_c_1",                         true},
{E_DAPC_MM_SLAVE,         137, "cam_raw_c_ip_group_0_inner",            true},

 /* 370 */
{E_DAPC_MM_SLAVE,         138, "cam_raw_c_ip_group_1_inner",            true},
{E_DAPC_MM_SLAVE,         139, "cam_raw_c_ip_group_2_inner",            true},
{E_DAPC_MM_SLAVE,         140, "cam_raw_c_ip_group_3_inner",            true},
{E_DAPC_MM_SLAVE,         141, "cam_raw_c_dma_0_inner",                 true},
{E_DAPC_MM_SLAVE,         142, "cam_raw_c_dma_1_inner",                 true},
{E_DAPC_MM_SLAVE,         143, "ltm_curve_c_0_inner",                   true},
{E_DAPC_MM_SLAVE,         144, "ltm_curve_c_1_inner",                   true},
{E_DAPC_MM_SLAVE,         145, "cam_raw_a_AA_histogram_0",              true},
{E_DAPC_MM_SLAVE,         146, "cam_raw_a_AA_histogram_1",              true},
{E_DAPC_MM_SLAVE,         147, "cam_raw_a_AA_histogram_2",              true},

 /* 380 */
{E_DAPC_MM_SLAVE,         148, "cam_raw_a_MAE_histogram",               true},
{E_DAPC_MM_SLAVE,         149, "cam_raw_b_AA_histogram_0",              true},
{E_DAPC_MM_SLAVE,         150, "cam_raw_b_AA_histogram_1",              true},
{E_DAPC_MM_SLAVE,         151, "cam_raw_b_AA_histogram_2",              true},
{E_DAPC_MM_SLAVE,         152, "cam_raw_b_MAE_histogram",               true},
{E_DAPC_MM_SLAVE,         153, "cam_raw_c_AA_histogram_0",              true},
{E_DAPC_MM_SLAVE,         154, "cam_raw_c_AA_histogram_1",              true},
{E_DAPC_MM_SLAVE,         155, "cam_raw_c_AA_histogram_2",              true},
{E_DAPC_MM_SLAVE,         156, "cam_raw_c_MAE_histogram",               true},
{E_DAPC_MM_SLAVE,         157, "CAMSYS_RESERVED",                       true},

 /* 390 */
{E_DAPC_MM_SLAVE,         158, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         159, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         160, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         161, "CAMSYS_adl",                            true},
{E_DAPC_MM_SLAVE,         162, "camsv_0",                               true},
{E_DAPC_MM_SLAVE,         163, "camsv_1",                               true},
{E_DAPC_MM_SLAVE,         164, "camsv_2",                               true},
{E_DAPC_MM_SLAVE,         165, "camsv_3",                               true},
{E_DAPC_MM_SLAVE,         166, "camsv_4",                               true},
{E_DAPC_MM_SLAVE,         167, "camsv_5",                               true},

 /* 400 */
{E_DAPC_MM_SLAVE,         168, "camsv_6",                               true},
{E_DAPC_MM_SLAVE,         169, "camsv_7",                               true},
{E_DAPC_MM_SLAVE,         170, "CAMSYS_asg",                            true},
{E_DAPC_MM_SLAVE,         171, "cam_raw_a_set",                         true},
{E_DAPC_MM_SLAVE,         172, "cam_raw_a_clr",                         true},
{E_DAPC_MM_SLAVE,         173, "cam_raw_b_set",                         true},
{E_DAPC_MM_SLAVE,         174, "cam_raw_b_clr",                         true},
{E_DAPC_MM_SLAVE,         175, "cam_raw_c_set",                         true},
{E_DAPC_MM_SLAVE,         176, "cam_raw_c_clr",                         true},
{E_DAPC_MM_SLAVE,         177, "CAMSYS_RESERVED",                       true},

 /* 410 */
{E_DAPC_MM_SLAVE,         178, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         179, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         180, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         181, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         182, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         183, "CAMSYS_tsf",                            true},
{E_DAPC_MM_SLAVE,         184, "ccu_ctl",                               true},
{E_DAPC_MM_SLAVE,         185, "ccu_h2t_a",                             true},
{E_DAPC_MM_SLAVE,         186, "ccu_t2h_a",                             true},
{E_DAPC_MM_SLAVE,         187, "ccu_dma",                               true},

 /* 420 */
{E_DAPC_MM_SLAVE,         188, "CAMSYS_RESERVED",                       true},
{E_DAPC_MM_SLAVE,         189, "DCCM",                                  true},
{E_DAPC_MM_SLAVE,         190, "CAMSYS_RESEVE",                         true},
{E_DAPC_MM_SLAVE,         191, "CAMSYS_OTHERS",                         true},
{E_DAPC_MM_SLAVE,         192, "APUSYS_apu_conn_config",                true},
{E_DAPC_MM_SLAVE,         193, "APUSYS_ADL_CTRL",                       true},
{E_DAPC_MM_SLAVE,         194, "APUSYS_vcore_config",                   true},
{E_DAPC_MM_SLAVE,         195, "APUSYS_apu_edma",                       true},
{E_DAPC_MM_SLAVE,         196, "APUSYS_apu1_edmb",                      true},
{E_DAPC_MM_SLAVE,         197, "APUSYS_apu2_edmc",                      true},

 /* 430 */
{E_DAPC_MM_SLAVE,         198, "APUSYS_COREA_DMEM_0",                   true},
{E_DAPC_MM_SLAVE,         199, "APUSYS_COREA_DMEM_1",                   true},
{E_DAPC_MM_SLAVE,         200, "APUSYS_COREA_IMEM",                     true},
{E_DAPC_MM_SLAVE,         201, "APUSYS_COREA_CONTROL",                  true},
{E_DAPC_MM_SLAVE,         202, "APUSYS_COREA_DEBUG",                    true},
{E_DAPC_MM_SLAVE,         203, "APUSYS_COREB_DMEM_0",                   true},
{E_DAPC_MM_SLAVE,         204, "APUSYS_COREB_DMEM_1",                   true},
{E_DAPC_MM_SLAVE,         205, "APUSYS_COREB_IMEM",                     true},
{E_DAPC_MM_SLAVE,         206, "APUSYS_COREB_CONTROL",                  true},
{E_DAPC_MM_SLAVE,         207, "APUSYS_COREB_DEBUG",                    true},

 /* 440 */
{E_DAPC_MM_SLAVE,         208, "APUSYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         209, "APUSYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         210, "APUSYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         211, "APUSYS_COREC_CONTROL",                  true},
{E_DAPC_MM_SLAVE,         212, "APUSYS_COREC_MDLA",                     true},
{E_DAPC_MM_SLAVE,         213, "APUSYS_OTHERS",                         true},
{E_DAPC_MM_SLAVE,         214, "IPESYS_ipesys_top",                     true},
{E_DAPC_MM_SLAVE,         215, "IPESYS_fdvt",                           true},
{E_DAPC_MM_SLAVE,         216, "IPESYS_fe",                             true},
{E_DAPC_MM_SLAVE,         217, "IPESYS_rsc",                            true},

 /* 450 */
{E_DAPC_MM_SLAVE,         218, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         219, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         220, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         221, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         222, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         223, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         224, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         225, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         226, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         227, "IPESYS_ RESERVED",                      true},

 /* 460 */
{E_DAPC_MM_SLAVE,         228, "IPESYS_smi_2x1_sub_common",             true},
{E_DAPC_MM_SLAVE,         229, "IPESYS_smi_larb8",                      true},
{E_DAPC_MM_SLAVE,         230, "IPESYS_depth",                          true},
{E_DAPC_MM_SLAVE,         231, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         232, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         233, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         234, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         235, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         236, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         237, "IPESYS_ RESERVED",                      true},

 /* 470 */
{E_DAPC_MM_SLAVE,         238, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         239, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         240, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         241, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         242, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         243, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         244, "IPESYS_ RESERVED",                      true},
{E_DAPC_MM_SLAVE,         245, "IPESYS_smi_larb7",                      true},
{E_DAPC_MM_SLAVE,         246, "IPESYS_OTHERS",                         true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SSPM_UNALIGN",                          true},

 /* 480 */
{E_DAPC_OTHERS_SLAVE,     -1,  "SSPM_OUT_OF_BOUND",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SSPM_ERR_WAY_EN",                       true},
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_PERIAPB_UNALIGN",                  true},
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_PERIAPB _OUT_OF_BOUND",            true},
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_PERIAPB _ERR_WAY_EN",              true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SOUTH_PERIAPB_UNALIGN",                 true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SOUTH_PERIAPB _OUT_OF_BOUND",           true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SOUTH_PERIAPB _ERR_WAY_EN",             true},
{E_DAPC_OTHERS_SLAVE,     -1,  "WEST_PERIAPB_UNALIGN",                  true},
{E_DAPC_OTHERS_SLAVE,     -1,  "WEST _PERIAPB _OUT_OF_BOUND",           true},

 /* 490 */
{E_DAPC_OTHERS_SLAVE,     -1,  "WEST _PERIAPB _ERR_WAY_EN",             true},
{E_DAPC_OTHERS_SLAVE,     -1,  "NORTH_PERIAPB_UNALIGN",                 true},
{E_DAPC_OTHERS_SLAVE,     -1,  "NORTH _PERIAPB _OUT_OF_BOUND",          true},
{E_DAPC_OTHERS_SLAVE,     -1,  "NORTH _PERIAPB _ERR_WAY_EN",            true},
{E_DAPC_OTHERS_SLAVE,     -1,  "INFRA_PDN_DECODE_ERROR",                true},
{E_DAPC_OTHERS_SLAVE,     -1,  "PERIAPB_UNALIGN",                       true},
{E_DAPC_OTHERS_SLAVE,     -1,  "PERIAPB_OUT_OF_BOUND",                  true},
{E_DAPC_OTHERS_SLAVE,     -1,  "PERIAPB_ERR_WAY_EN",                    true},
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_NORTH_PERIAPB_UNALIGN",            true},
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_NORTH _PERIAPB _OUT_OF_BOUND",     true},

 /* 500 */
{E_DAPC_OTHERS_SLAVE,     -1,  "EAST_NORTH _PERIAPB _ERR_WAY_EN",       true},
{E_DAPC_OTHERS_SLAVE,     -1,  "TOPAXI_SI2_DECERR",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "TOPAXI_SI1_DECERR",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "TOPAXI_SI0_DECERR",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "PERIAXI_SI0_DECERR",                    true},
{E_DAPC_OTHERS_SLAVE,     -1,  "PERIAXI_SI1_DECERR",                    true},
{E_DAPC_OTHERS_SLAVE,     -1,  "TOPAXI_SI3_DECERR",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "TOPAXI_SI4_DECERR",                     true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SRAMROM_DECERR",                        true},
{E_DAPC_OTHERS_SLAVE,     -1,  "HIFI3_DECERR",                          true},

 /* 510 */
{E_DAPC_OTHERS_SLAVE,     -1,  "MD_DECERR",                             true},
{E_DAPC_OTHERS_SLAVE,     -1,  "SRAMROM*",                              true},
{E_DAPC_OTHERS_SLAVE,     -1,  "AP_DMA*",                               false},
{E_DAPC_OTHERS_SLAVE,     -1,  "DEVICE_APC_AO_INFRA_PERI*",             false},
{E_DAPC_OTHERS_SLAVE,     -1,  "DEVICE_APC_AO_MD*",                     false},
{E_DAPC_OTHERS_SLAVE,     -1,  "DEVICE_APC_AO_MM*",                     false},
{E_DAPC_OTHERS_SLAVE,     -1,  "CM_DQ_SECURE*",                         false},
{E_DAPC_OTHERS_SLAVE,     -1,  "MM_IOMMU_DOMAIN*",                      false},
{E_DAPC_OTHERS_SLAVE,     -1,  "DISP_GCE*",                             false},
{E_DAPC_OTHERS_SLAVE,     -1,  "DEVICE_APC*",                           false},

 /* 520 */
{E_DAPC_OTHERS_SLAVE,     -1,  "EMI*",                                  false},
{E_DAPC_OTHERS_SLAVE,     -1,  "EMI_MPU*",                              false},
{E_DAPC_OTHERS_SLAVE,     -1,  "PMIC_WRAP*",                            false},
{E_DAPC_OTHERS_SLAVE,     -1,  "I2C1*",                                 false},
{E_DAPC_OTHERS_SLAVE,     -1,  "I2C2*",                                 false},
{E_DAPC_OTHERS_SLAVE,     -1,  "I2C4*",                                 false},

};
#endif


/**************************************************************************
 *STATIC FUNCTION
 **************************************************************************/

#ifdef CONFIG_MTK_HIBERNATION
static int devapc_pm_restore_noirq(struct device *device)
{
	if (devapc_infra_irq != 0) {
		mt_irq_set_sens(devapc_infra_irq, MT_LEVEL_SENSITIVE);
		mt_irq_set_polarity(devapc_infra_irq, MT_POLARITY_LOW);
	}

	return 0;
}
#endif

#if DEVAPC_TURN_ON
static void unmask_infra_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_MASK_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_MASK(apc_index) &=
		(0xFFFFFFFF ^ (1 << apc_bit_index));
}

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
static void mask_infra_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_MASK_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_MASK(apc_index) |= (1 << apc_bit_index);
}
#endif

static int clear_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_STA_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return -1;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_STA(apc_index) = (0x1 << apc_bit_index);

	return 0;
}

static int check_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_STA_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return -1;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (*DEVAPC_PD_INFRA_VIO_STA(apc_index) & (0x1 << apc_bit_index))
		return 1;

	return 0;
}

static void print_vio_mask_sta(void)
{
	DEVAPC_DBG_MSG("[DEVAPC] %s 0:0x%x 1:0x%x 2:0x%x 3:0x%x\n",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(0)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(1)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(2)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(3)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 4:0x%x 5:0x%x 6:0x%x 7:0x%x 8:0x%x\n",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(4)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(5)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(6)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(7)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(8)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 9:0x%x 10:0x%x 11:0x%x 12:0x%x\n",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(9)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(10)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(11)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(12)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 13:0x%x 14:0x%x 15:0x%x 16:0x%x\n",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(13)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(14)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(15)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(16)));

	DEVAPC_DBG_MSG("[DEVAPC] %s 0:0x%x 1:0x%x 2:0x%x 3:0x%x\n",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(0)),
			readl(DEVAPC_PD_INFRA_VIO_STA(1)),
			readl(DEVAPC_PD_INFRA_VIO_STA(2)),
			readl(DEVAPC_PD_INFRA_VIO_STA(3)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 4:0x%x 5:0x%x 6:0x%x 7:0x%x 8:0x%x\n",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(4)),
			readl(DEVAPC_PD_INFRA_VIO_STA(5)),
			readl(DEVAPC_PD_INFRA_VIO_STA(6)),
			readl(DEVAPC_PD_INFRA_VIO_STA(7)),
			readl(DEVAPC_PD_INFRA_VIO_STA(8)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 9:0x%x 10:0x%x 11:0x%x 12:0x%x\n",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(9)),
			readl(DEVAPC_PD_INFRA_VIO_STA(10)),
			readl(DEVAPC_PD_INFRA_VIO_STA(11)),
			readl(DEVAPC_PD_INFRA_VIO_STA(12)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 13:0x%x 14:0x%x 15:0x%x 16:0x%x\n",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(13)),
			readl(DEVAPC_PD_INFRA_VIO_STA(14)),
			readl(DEVAPC_PD_INFRA_VIO_STA(15)),
			readl(DEVAPC_PD_INFRA_VIO_STA(16)));
}

static void start_devapc(void)
{
	unsigned int i;
	uint32_t vio_shift_sta;

	DEVAPC_MSG("[DEVAPC] %s\n", __func__);

	mt_reg_sync_writel(0x80000000, DEVAPC_PD_INFRA_APC_CON);
	print_vio_mask_sta();

	DEVAPC_DBG_MSG("[DEVAPC] %s\n",
		"Clear INFRA VIO_STA and unmask INFRA VIO_MASK...");

	vio_shift_sta = readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA);
	if (vio_shift_sta) {
		DEVAPC_DBG_MSG("[DEVAPC] (Pre) clear VIO_SHIFT_STA = 0x%x\n",
			vio_shift_sta);
		mt_reg_sync_writel(vio_shift_sta,
			DEVAPC_PD_INFRA_VIO_SHIFT_STA);
		DEVAPC_DBG_MSG("[DEVAPC] (Post) clear VIO_SHIFT_STA = 0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));
	}

	for (i = 0; i < ARRAY_SIZE(devapc_infra_devices); i++)
		if (true == devapc_infra_devices[i].enable_vio_irq) {
			clear_infra_vio_status(i);
			unmask_infra_module_irq(i);
		}

	print_vio_mask_sta();

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
	devapc_vio_current_aee_trigger_times = 0;
#endif

}

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)

/* violation index corresponds to subsys */
static const char *index_to_subsys(unsigned int index)
{
	if (index >= 232 && index <= 244)
		return "MFGSYS";
	else if ((index >= 268 && index <= 270) || index == 294 || index == 295
		|| index == 298 || index == 307 || index == 325 || index == 460
		|| index == 461 || index == 477)
		return "SMI";
	else if ((index >= 253 && index <= 263) || index == 271 || index == 278)
		return "MMSYS_DISP";
	else if (index >= 244 && index <= 279)
		return "MMSYS";
	else if (index >= 280 && index <= 296)
		return "IMGSYS";
	else if (index >= 297 && index <= 305)
		return "VENCSYS";
	else if (index >= 306 && index <= 312)
		return "VDECSYS";
	else if (index >= 313 && index <= 423)
		return "CAMSYS";
	else if (index >= 424 && index <= 445)
		return "VPUSYS";
	else if (index >= 446 && index <= 478)
		return "IPESYS";
	else if (index < ARRAY_SIZE(devapc_infra_devices))
		return devapc_infra_devices[index].device;
	else
		return "OUT_OF_BOUND";
}

static void execute_aee(unsigned int i, unsigned int domain_id,
			unsigned int dbg1)
{
	char subsys_str[48] = {0};
	const char *mtk_platform = "mt6779";

	DEVAPC_MSG("[DEVAPC] Executing AEE Exception...\n");
	DEVAPC_MSG("[DEVAPC] aee_trigger_times: %d\n",
		devapc_vio_current_aee_trigger_times);

	/* mask irq for module "i" */
	mask_infra_module_irq(i);

	devapc_vio_current_aee_trigger_times++;

	/* Domain view is different from Infra-Peri/MM/MD */
	if (i == DEVAPC_MD_VIO) {
		strncpy(subsys_str, index_to_subsys(i),
			sizeof(subsys_str));
	} else if (i >= DEVAPC_MM_VIO_START) {
		if (domain_id == 2)
			strncpy(subsys_str, "SSPM", sizeof(subsys_str));
		else
			strncpy(subsys_str, index_to_subsys(i),
				sizeof(subsys_str));
	} else {
		if (domain_id == 1) {
			strncpy(subsys_str, "MD_SI", sizeof(subsys_str));
		} else if (domain_id == 3) {
			strncpy(subsys_str, "SCP", sizeof(subsys_str));
		} else if (domain_id == 8) {
			strncpy(subsys_str, "SSPM", sizeof(subsys_str));
		} else if (domain_id == 9) {
			strncpy(subsys_str, "SPM", sizeof(subsys_str));
		} else if (domain_id == 10) {
			strncpy(subsys_str, "ADSP", sizeof(subsys_str));
		} else if (domain_id == 11) {
			strncpy(subsys_str, "GZ", sizeof(subsys_str));
		} else {
			strncpy(subsys_str, index_to_subsys(i),
				sizeof(subsys_str));
		}
	}

	strncat(subsys_str, "_", 1);
	strncat(subsys_str, mtk_platform, strlen(mtk_platform));
	subsys_str[sizeof(subsys_str)-1] = '\0';

	aee_kernel_exception("DEVAPC",
			"%s %s, Vio Addr: 0x%x\n%s%s\n",
			"[DEVAPC] Violation Slave:",
			devapc_infra_devices[i].device,
			dbg1,
			"CRDISPATCH_KEY:Device APC Violation Issue/",
			subsys_str
			);

	/* unmask irq for module "i" */
	unmask_infra_module_irq(i);

}
#endif // AEE_FEATURE

static unsigned int sync_vio_dbg(int shift_bit)
{
	unsigned int shift_count = 0;
	unsigned int sync_done;

	mt_reg_sync_writel(0x1 << shift_bit, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
	mt_reg_sync_writel(0x1,	DEVAPC_PD_INFRA_VIO_SHIFT_CON);

	for (shift_count = 0; (shift_count < 100) &&
		((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) != 0x3);
		++shift_count)
		DEVAPC_DBG_MSG("[DEVAPC] Syncing INFRA DBG0 & DBG1 (%d, %d)\n",
				shift_bit, shift_count);

	DEVAPC_DBG_MSG("[DEVAPC] VIO_SHIFT_SEL=0x%X, VIO_SHIFT_CON=0x%X\n",
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL),
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));

	if ((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) == 0x3)
		sync_done = 1;
	else {
		sync_done = 0;
		DEVAPC_MSG("[DEVAPC] sync failed, shift_bit: %d\n",
				shift_bit);
	}

	/* disable shift mechanism */
	mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_CON);
	mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
	mt_reg_sync_writel(0x1 << shift_bit, DEVAPC_PD_INFRA_VIO_SHIFT_STA);

	return sync_done;
}

#ifdef DBG_ENABLE
static void dump_backtrace(void *passed_regs)
{
	struct pt_regs *regs = passed_regs;

	if ((DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG)
		|| (enable_dynamic_one_core_violation_debug)) {
		DEVAPC_MSG("[DEVAPC] ====== %s ======\n",
			"Start dumping Device APC violation tracing");

		DEVAPC_MSG("[DEVAPC] ****** %s ******\n",
			"[All IRQ Registers]");
		if (regs)
			show_regs(regs);

		DEVAPC_MSG("[DEVAPC] ****** %s ******\n",
			"[All Current Task Stack]");
		show_stack(current, NULL);

		DEVAPC_MSG("[DEVAPC] ====== %s ======\n",
			"End of dumping Device APC violation tracing");
	}
}
#endif

static char *perm_to_string(uint32_t perm)
{
	if (perm == 0x0)
		return "NO_PROTECTION";
	else if (perm == 0x1)
		return "SECURE_RW_ONLY";
	else if (perm == 0x2)
		return "SECURE_RW_NS_R_ONLY";
	else if (perm == 0x3)
		return "FORBIDDEN";
	else
		return "UNKNOWN_PERM";
}

static uint32_t get_permission(int vio_index, int domain)
{
	int slave_type;
	int config_idx;
	int apc_set_idx;
	uint32_t ret;

	slave_type = devapc_infra_devices[vio_index].DEVAPC_SLAVE_TYPE;
	config_idx = devapc_infra_devices[vio_index].config_index;

	DEVAPC_DBG_MSG("%s, slave type = 0x%x, config_idx = 0x%x\n",
			__func__,
			slave_type,
			config_idx);

	if (slave_type >= E_DAPC_OTHERS_SLAVE || config_idx == -1) {
		DEVAPC_DBG_MSG("%s, cannot get APC\n", __func__);
		return 0xdead;
	}

	ret = mt_secure_call(MTK_SIP_KERNEL_DAPC_DUMP, slave_type,
			domain, config_idx, 0);

	DEVAPC_DBG_MSG("%s, dump perm = 0x%x\n", __func__, ret);

	apc_set_idx = config_idx % MOD_NO_IN_1_DEVAPC;
	ret = (ret & (0x3 << (apc_set_idx * 2))) >> (apc_set_idx * 2);

	DEVAPC_DBG_MSG("%s, after shipping, dump perm = 0x%x\n",
			__func__,
			(ret & 0x3));

	return (ret & 0x3);
}

uint32_t devapc_vio_check(void)
{
	return readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA);
}
EXPORT_SYMBOL(devapc_vio_check);

void dump_dbg_info(void)
{
	unsigned int dbg0 = 0;
	unsigned int write_violation;
	unsigned int read_violation;
	unsigned int vio_addr_high;
	int i;

	DEVAPC_DBG_MSG("[DEVAPC] VIO_SHIFT_STA: 0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));

	for (i = 0; i <= PD_INFRA_VIO_SHIFT_MAX_BIT; ++i) {
		if (devapc_vio_check() & (0x1 << i)) {

			if (sync_vio_dbg(i) == 0)
				continue;

			DEVAPC_DBG_MSG("[DEVAPC] %s%X, %s%X, %s%X\n",
					"VIO_SHIFT_STA=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA),
					"VIO_SHIFT_SEL=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL),
					"VIO_SHIFT_CON=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));

			dbg0 = readl(DEVAPC_PD_INFRA_VIO_DBG0);
			dbg1 = readl(DEVAPC_PD_INFRA_VIO_DBG1);

			master_id = (dbg0 & INFRA_VIO_DBG_MSTID)
				>> INFRA_VIO_DBG_MSTID_START_BIT;
			domain_id = (dbg0 & INFRA_VIO_DBG_DMNID)
				>> INFRA_VIO_DBG_DMNID_START_BIT;
			write_violation = (dbg0 & INFRA_VIO_DBG_W_VIO)
				>> INFRA_VIO_DBG_W_VIO_START_BIT;
			read_violation = (dbg0 & INFRA_VIO_DBG_R_VIO)
				>> INFRA_VIO_DBG_R_VIO_START_BIT;
			vio_addr_high = (dbg0 & INFRA_VIO_ADDR_HIGH)
				>> INFRA_VIO_ADDR_HIGH_START_BIT;

			/* violation information */
			DEVAPC_MSG("%s%s%s%s%x %s%x, %s%x, %s%x\n",
					"[DEVAPC] Violation(",
					read_violation == 1?" R":"",
					write_violation == 1?" W ) - ":" ) - ",
					"Vio Addr:0x", dbg1,
					"High:0x", vio_addr_high,
					"Bus ID:0x", master_id,
					"Dom ID:0x", domain_id);

			DEVAPC_MSG("%s - %s%s, %s%i\n",
					"[DEVAPC] Violation",
					"Process:", current->comm,
					"PID:", current->pid);
		}
	}

}
EXPORT_SYMBOL(dump_dbg_info);

static irqreturn_t devapc_violation_irq(int irq_number, void *dev_id)
{
	unsigned int device_count;
	unsigned int i;
	uint32_t perm;
#ifdef DBG_ENABLE
	struct pt_regs *regs = get_irq_regs();
#endif

	if (irq_number != devapc_infra_irq) {
		DEVAPC_MSG("[DEVAPC] (ERROR) irq_number %d %s\n",
				irq_number,
				"is not registered!");

		return IRQ_NONE;
	}
	print_vio_mask_sta();
	dump_dbg_info();

	device_count = ARRAY_SIZE(devapc_infra_devices);

	/* checking and showing violation normal slaves */
	for (i = 0; i < device_count; i++) {
		if (devapc_infra_devices[i].enable_vio_irq == true
			&& check_infra_vio_status(i) == 1) {

			if (i == DEVAPC_SRAMROM_VIO_INDEX) {
				DEVAPC_MSG("%s %s\n", "[DEVAPC]",
					"clear SRAMROM VIO");
				mt_secure_call(MTK_SIP_KERNEL_CLR_SRAMROM_VIO,
				0, 0, 0, 0);
			}
			clear_infra_vio_status(i);
			perm = get_permission(i, domain_id);
			DEVAPC_MSG("%s %s\n",
				"[DEVAPC] Violation Master:",
				bus_id_to_master(master_id, dbg1, i));
			DEVAPC_MSG("%s %s %s (%s=%d)\n",
				"[DEVAPC]",
				"Access Violation Slave:",
				devapc_infra_devices[i].device,
				"infra index",
				i);
			DEVAPC_MSG("%s %s %s\n",
				"[DEVAPC]",
				"permission:",
				perm_to_string(perm));

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
			execute_aee(i, domain_id, dbg1);
#endif
		}
	}

#ifdef DBG_ENABLE
	dump_backtrace(regs);
#endif
	return IRQ_HANDLED;
}
#endif //DEVAPC_TURN_ON

static int devapc_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
#if DEVAPC_TURN_ON
	int ret;
#endif

	DEVAPC_MSG("[DEVAPC] %s\n", __func__);

	if (devapc_pd_infra_base == NULL) {
		if (node) {
			devapc_pd_infra_base = of_iomap(node,
				DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			devapc_ao_infra_base = of_iomap(node,
				DAPC_DEVICE_TREE_NODE_AO_INFRA_INDEX);
			devapc_infra_irq = irq_of_parse_and_map(node,
				DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			DEVAPC_MSG("[DEVAPC] PD_INFRA_ADDRESS: %p, IRQ: %d\n",
				devapc_pd_infra_base, devapc_infra_irq);
		} else {
			pr_info("[DEVAPC] %s\n",
				"can't find DAPC_INFRA_PD compatible node");
			return -1;
		}
	}

#if DEVAPC_TURN_ON
	ret = request_irq(devapc_infra_irq, (irq_handler_t)devapc_violation_irq,
			IRQF_TRIGGER_LOW | IRQF_SHARED,
			"devapc", &g_devapc_ctrl);
	if (ret) {
		pr_info("[DEVAPC] Failed to request infra irq! (%d)\n", ret);
		return ret;
	}
#endif

	/* CCF */
#if DEVAPC_USE_CCF
	dapc_infra_clk = devm_clk_get(&pdev->dev, "devapc-infra-clock");
	if (IS_ERR(dapc_infra_clk)) {
		pr_info("[DEVAPC] (Infra) %s\n",
			"Cannot get devapc clock from common clock framework.");
		return PTR_ERR(dapc_infra_clk);
	}
	clk_prepare_enable(dapc_infra_clk);
#endif

#ifdef CONFIG_MTK_HIBERNATION
	register_swsusp_restore_noirq_func(ID_M_DEVAPC,
		devapc_pm_restore_noirq, NULL);
#endif

#if DEVAPC_TURN_ON
	start_devapc();
#endif

	return 0;
}

static int devapc_remove(struct platform_device *dev)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	clk_disable_unprepare(dapc_infra_clk);
	return 0;
}

static int devapc_suspend(struct platform_device *dev, pm_message_t state)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

static int devapc_resume(struct platform_device *dev)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

#ifdef DBG_ENABLE
static ssize_t devapc_dbg_read(struct file *file, char __user *buffer,
	size_t count, loff_t *ppos)
{
	ssize_t retval = 0;
	char msg[256] = "DBG: test violation...\n";

	if (*ppos >= strlen(msg))
		return 0;

	DEVAPC_MSG("[DEVAPC] %s, test violation...\n", __func__);

	retval = simple_read_from_buffer(buffer, count, ppos, msg, strlen(msg));

	DEVAPC_MSG("[DEVAPC] %s, devapc_ao_infra_base = 0x%x\n",
			__func__,
			readl((unsigned int *)(devapc_ao_infra_base + 0x0)));
	DEVAPC_MSG("[DEVAPC] %s, test done, it should generate violation!\n",
			__func__);

	return retval;
}

static ssize_t devapc_dbg_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char input[32];
	char *pinput = NULL;
	char *tmp = NULL;
	long i;
	int len = 0;
	int apc_set_idx;
	uint32_t ret;
	long slave_type = 0, domain = 0, index = 0;

	DEVAPC_MSG("[DEVAPC] debugging...\n");
	len = (count < (sizeof(input) - 1)) ? count : (sizeof(input) - 1);
	if (copy_from_user(input, buffer, len)) {
		pr_info("[DEVAPC] copy from user failed!\n");
		return -EFAULT;
	}

	input[len] = '\0';
	pinput = input;

	if (sysfs_streq(input, "0")) {
		enable_dynamic_one_core_violation_debug = 0;
		DEVAPC_MSG("[DEVAPC] One-Core Debugging: Disabled\n");
	} else if (sysfs_streq(input, "1")) {
		enable_dynamic_one_core_violation_debug = 1;
		DEVAPC_MSG("[DEVAPC] One-Core Debugging: Enabled\n");
	} else {
		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &slave_type);
		else
			slave_type = E_DAPC_OTHERS_SLAVE;

		if (slave_type >= E_DAPC_OTHERS_SLAVE) {
			pr_info("[DEVAPC] wrong input slave type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] slave_type = %lu\n", slave_type);

		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &domain);
		else
			domain = E_DOMAIN_OTHERS;

		if (domain >= E_DOMAIN_OTHERS) {
			pr_info("[DEVAPC] wrong input domain type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] domain id = %lu\n", domain);

		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &index);
		else
			index = 0xFFFFFFFF;

		if (index > DEVAPC_TOTAL_SLAVES) {
			pr_info("[DEVAPC] wrong input index type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] slave config_idx = %lu\n", index);

		ret = mt_secure_call(MTK_SIP_KERNEL_DAPC_DUMP, slave_type,
					domain, index, 0);

		DEVAPC_MSG("[DEVAPC] dump perm = 0x%x\n", ret);

		apc_set_idx = index % MOD_NO_IN_1_DEVAPC;
		ret = (ret & (0x3 << (apc_set_idx * 2))) >> (apc_set_idx * 2);

		DEVAPC_MSG("%s the permission is %s\n",
			"[DEVAPC]",
			perm_to_string((ret & 0x3)));
	}

	return count;
}
#endif

static int devapc_dbg_open(struct inode *inode, struct file *file)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

static const struct file_operations devapc_dbg_fops = {
	.owner = THIS_MODULE,
	.open  = devapc_dbg_open,
#ifdef DBG_ENABLE
	.write = devapc_dbg_write,
	.read = devapc_dbg_read,
#else
	.write = NULL,
	.read = NULL,
#endif
};

static const struct of_device_id plat_devapc_dt_match[] = {
	{ .compatible = "mediatek,devapc" },
	{},
};

static struct platform_driver devapc_driver = {
	.probe = devapc_probe,
	.remove = devapc_remove,
	.suspend = devapc_suspend,
	.resume = devapc_resume,
	.driver = {
		.name = "devapc",
		.owner = THIS_MODULE,
		.of_match_table	= plat_devapc_dt_match,
	},
};

/*
 * devapc_init: module init function.
 */
static int __init devapc_init(void)
{
	int ret;

	DEVAPC_MSG("[DEVAPC] kernel module init.\n");

	ret = platform_driver_register(&devapc_driver);
	if (ret) {
		pr_info("[DEVAPC] Unable to register driver (%d)\n", ret);
		return ret;
	}

	g_devapc_ctrl = cdev_alloc();
	if (!g_devapc_ctrl) {
		pr_info("[DEVAPC] Failed to add devapc device! (%d)\n", ret);
		platform_driver_unregister(&devapc_driver);
		return ret;
	}
	g_devapc_ctrl->owner = THIS_MODULE;

	proc_create("devapc_dbg", 0664, NULL, &devapc_dbg_fops);
	/* 0664: (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) */

	return 0;
}

/*
 * devapc_exit: module exit function.
 */
static void __exit devapc_exit(void)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_DEVAPC);
#endif
}

arch_initcall(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
