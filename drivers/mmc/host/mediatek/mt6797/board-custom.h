#ifndef __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H
#define __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H

#include <generated/autoconf.h>

/*=======================================================================*/
/* MT6797 MSDC                                                             */
/*=======================================================================*/
#ifdef CONFIG_FPGA_EARLY_PORTING
#define CFG_DEV_MSDC0
#else
#ifdef CONFIG_MTK_EMMC_SUPPORT
#define CFG_DEV_MSDC0
#endif
#define CFG_DEV_MSDC1
#endif


#if defined(CONFIG_MTK_C2K_SUPPORT)
#define CFG_DEV_MSDC3
#endif

/*
SDIO slot index number used by connectivity combo chip: may be MSDC0~4
*/
#if defined(CONFIG_MTK_COMBO) || defined(CONFIG_MTK_COMBO_MODULE)
#define CONFIG_MTK_COMBO_SDIO_SLOT  (3)
#else
#undef CONFIG_MTK_COMBO_SDIO_SLOT
#endif

#define NFI_DEFAULT_ACCESS_TIMING        (0x44333)

/*uboot only support 1 cs*/
#define NFI_CS_NUM                  (2)
#define NFI_DEFAULT_CS				(0)

#define USE_AHB_MODE                (1)

#define PLATFORM_EVB                (1)

#endif				/* __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H */
