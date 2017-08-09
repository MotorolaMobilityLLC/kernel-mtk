#ifndef __ARCH_ARM_MACH_BOARD_H
#define __ARCH_ARM_MACH_BOARD_H

#include <generated/autoconf.h>
#include <linux/pm.h>
/* weiping fix */
#if 0
#include <board-custom.h>
#else
#include "board-custom.h"
#endif

typedef void (*sdio_irq_handler_t)(void *);	/* external irq handler */
typedef void (*pm_callback_t)(pm_message_t state, void *data);

#define MSDC_CD_PIN_EN      (1 << 0)	/* card detection pin is wired   */
#define MSDC_WP_PIN_EN      (1 << 1)	/* write protection pin is wired */
#define MSDC_RST_PIN_EN     (1 << 2)	/* emmc reset pin is wired       */
#define MSDC_SDIO_IRQ       (1 << 3)	/* use internal sdio irq (bus)   */
#define MSDC_EXT_SDIO_IRQ   (1 << 4)	/* use external sdio irq         */
#define MSDC_REMOVABLE      (1 << 5)	/* removable slot                */
#define MSDC_SYS_SUSPEND    (1 << 6)	/* suspended by system           */
#define MSDC_HIGHSPEED      (1 << 7)	/* high-speed mode support       */
#define MSDC_UHS1           (1 << 8)	/* uhs-1 mode support            */
#define MSDC_DDR            (1 << 9)	/* ddr mode support              */
#define MSDC_INTERNAL_CLK   (1 << 11)	/* Force Internal clock          */
#ifdef CONFIG_MTK_EMMC_CACHE
#define MSDC_CACHE          (1 << 12)	/* eMMC cache feature            */
#endif
#define MSDC_HS400          (1 << 13)	/* HS400 speed mode support      */

#define MSDC_SD_NEED_POWER  (1 << 31)	/* for Yecon board, need SD power always on!! or cannot recognize the sd card */

#define MSDC_SMPL_RISING    (0)
#define MSDC_SMPL_FALLING   (1)

#define MSDC_CMD_PIN        (0)
#define MSDC_DAT_PIN        (1)
#define MSDC_CD_PIN         (2)
#define MSDC_WP_PIN         (3)
#define MSDC_RST_PIN        (4)

#define MSDC_DATA1_INT      (1)

/* each PLL have different gears for select
 * software can used mux interface from clock management module to select */
enum {
	MSDC50_CLKSRC_26MHZ = 0,
	MSDC50_CLKSRC_800MHZ,
	MSDC50_CLKSRC_400MHZ,
	MSDC50_CLKSRC_200MHZ,
	MSDC50_CLKSRC_182MHZ,
	MSDC50_CLKSRC_136MHZ,
	MSDC50_CLKSRC_156MHZ,
	MSDC50_CLKSRC_416MHZ,
	MSDC50_CLKSRC_48MHZ,
	MSDC50_CLKSRC_91MHZ,
	MSDC50_CLKSRC_624MHZ
};

enum {
	MSDC30_CLKSRC_26MHZ = 0,
	MSDC30_CLKSRC_208MHZ,
	MSDC30_CLKSRC_200MHZ,
	MSDC30_CLKSRC_182MHZ,
	MSDC30_CLKSRC_136MHZ,
	MSDC30_CLKSRC_156MHZ,
	MSDC30_CLKSRC_48MHZ,
	MSDC30_CLKSRC_91MHZ
};

#define MSDC_BOOT_EN        (1)
#define MSDC_CD_HIGH        (1)
#define MSDC_CD_LOW         (0)
enum {
	MSDC_EMMC = 0,
	MSDC_SD = 1,
	MSDC_SDIO = 2
};

struct msdc_ett_settings {
#define MSDC_DEFAULT_MODE (0)
#define MSDC_SDR50_MODE   (1)
#define MSDC_DDR50_MODE   (2)
#define MSDC_HS200_MODE   (3)
#define MSDC_HS400_MODE   (4)
	unsigned int reg_addr;
	unsigned int reg_offset;
	unsigned int value;
};


extern struct msdc_hw msdc0_hw;
extern struct msdc_hw msdc1_hw;
extern struct msdc_hw msdc2_hw;
extern struct msdc_hw msdc3_hw;

/*GPS driver*/
#define GPS_FLAG_FORCE_OFF  0x0001
struct mt3326_gps_hardware {
	int (*ext_power_on)(int);
	int (*ext_power_off)(int);
};
extern struct mt3326_gps_hardware mt3326_gps_hw;

/* NAND driver */
struct mtk_nand_host_hw {
	unsigned int nfi_bus_width;	/* NFI_BUS_WIDTH */
	unsigned int nfi_access_timing;	/* NFI_ACCESS_TIMING */
	unsigned int nfi_cs_num;	/* NFI_CS_NUM */
	unsigned int nand_sec_size;	/* NAND_SECTOR_SIZE */
	unsigned int nand_sec_shift;	/* NAND_SECTOR_SHIFT */
	unsigned int nand_ecc_size;
	unsigned int nand_ecc_bytes;
	unsigned int nand_ecc_mode;
};
extern struct mtk_nand_host_hw mtk_nand_hw;

#endif				/* __ARCH_ARM_MACH_BOARD_H */
