#ifndef _MT_SPM_
#define _MT_SPM_

#include <linux/kernel.h>
#include <linux/io.h>
#include <mach/upmu_hw.h>	/* for PMIC power settings */

#ifdef CONFIG_OF
extern void __iomem *spm_base;
extern void __iomem *scp_i2c0_base;
extern void __iomem *scp_i2c1_base;
extern void __iomem *scp_i2c2_base;
extern void __iomem *spm_infracfg_ao_base;
extern void __iomem *spm_cksys_base;
extern void __iomem *spm_mcucfg;
extern void __iomem *spm_ddrphy_base;
extern u32 spm_irq_0;
extern u32 spm_irq_1;
extern u32 spm_irq_2;
extern u32 spm_irq_3;
extern u32 spm_irq_4;
extern u32 spm_irq_5;
extern u32 spm_irq_6;
extern u32 spm_irq_7;

#undef SPM_BASE
#define SPM_BASE spm_base
#else
#include <mach/mt_reg_base.h>
#endif

/* #include <mach/mt_irq.h> */
#include <mt-plat/sync_write.h>
#include <mt-plat/mt_io.h>

/**************************************
 * Config and Parameter
 **************************************/

/* FIXME: for bring up */
#ifdef CONFIG_OF
#undef SPM_I2C0_BASE
#undef SPM_I2C1_BASE
#undef SPM_I2C2_BASE
#define SPM_I2C0_BASE	scp_i2c0_base
#define SPM_I2C1_BASE	scp_i2c1_base
#define SPM_I2C2_BASE	scp_i2c2_base

#define SPM_IRQ0_ID		spm_irq_0
#define SPM_IRQ1_ID		spm_irq_1
#define SPM_IRQ2_ID		spm_irq_2
#define SPM_IRQ3_ID		spm_irq_3
#define SPM_IRQ4_ID		spm_irq_4
#define SPM_IRQ5_ID		spm_irq_5
#define SPM_IRQ6_ID		spm_irq_6
#define SPM_IRQ7_ID		spm_irq_7
#else
#define SPM_BASE		SLEEP_BASE

#define SPM_I2C0_BASE	0xF0059C00	/* SCP_I2C0_BASE */
#define SPM_I2C1_BASE	0xF0059C00	/* SCP_I2C1_BASE */
#define SPM_I2C2_BASE	0xF0059C00	/* SCP_I2C2_BASE */

#define SPM_IRQ0_ID		195	/* SLEEP_IRQ_BIT0_ID */
#define SPM_IRQ1_ID		196	/* SLEEP_IRQ_BIT1_ID */
#define SPM_IRQ2_ID		197	/* SLEEP_IRQ_BIT2_ID */
#define SPM_IRQ3_ID		198	/* SLEEP_IRQ_BIT3_ID */
#define SPM_IRQ4_ID		199	/* SLEEP_IRQ_BIT4_ID */
#define SPM_IRQ5_ID		200	/* SLEEP_IRQ_BIT5_ID */
#define SPM_IRQ6_ID		201	/* SLEEP_IRQ_BIT6_ID */
#define SPM_IRQ7_ID		202	/* SLEEP_IRQ_BIT7_ID */
#endif

#include "mt_spm_reg.h"

#define SPM_WAKE_SRC_LIST	{	\
	SPM_WAKE_SRC(0, SPM_MERGE),	/* PCM timer, TWAM or CPU */	\
	SPM_WAKE_SRC(1, MD32_WDT),	\
	SPM_WAKE_SRC(2, KP),		\
	SPM_WAKE_SRC(3, WDT),		\
	SPM_WAKE_SRC(4, GPT),		\
	SPM_WAKE_SRC(5, CONN2AP),	\
	SPM_WAKE_SRC(6, EINT),		\
	SPM_WAKE_SRC(7, CONN_WDT),	\
	SPM_WAKE_SRC(8, CCIF0_MD),	\
	SPM_WAKE_SRC(9, LOW_BAT),	\
	SPM_WAKE_SRC(10, MD32_SPM),	\
	SPM_WAKE_SRC(11, F26M_WAKE),	\
	SPM_WAKE_SRC(12, F26M_SLEEP),	\
	SPM_WAKE_SRC(13, PCM_WDT),	\
	SPM_WAKE_SRC(14, USB_CD),	\
	SPM_WAKE_SRC(15, USB_PDN),	\
	SPM_WAKE_SRC(16, C2K_WDT),	\
	SPM_WAKE_SRC(17, EINT_SECURE),	\
	SPM_WAKE_SRC(18, CCIF1_MD),	\
	SPM_WAKE_SRC(19, UART0),	\
	SPM_WAKE_SRC(20, AFE),		\
	SPM_WAKE_SRC(21, THERM),	\
	SPM_WAKE_SRC(22, CIRQ),		\
	SPM_WAKE_SRC(23, MD2_WDT),	\
	SPM_WAKE_SRC(24, SYSPWREQ),	\
	SPM_WAKE_SRC(25, MD_WDT),	\
	SPM_WAKE_SRC(26, CLDMA_MD),	\
	SPM_WAKE_SRC(27, SEJ),		\
	SPM_WAKE_SRC(28, ALL_MD32),	\
	SPM_WAKE_SRC(29, CPU_IRQ),	\
	SPM_WAKE_SRC(30, APSRC_WAKE),	\
	SPM_WAKE_SRC(31, APSRC_SLEEP)	\
}

/* define WAKE_SRC_XXX */
enum SPM_WAKE_SRC {
	WAKE_SRC_SPM_MERGE = (1U << 0),
	WAKE_SRC_MD32_WDT = (1U << 1),
	WAKE_SRC_KP = (1U << 2),
	WAKE_SRC_WDT = (1U << 3),
	WAKE_SRC_GPT = (1U << 4),
	WAKE_SRC_CONN2AP = (1U << 5),
	WAKE_SRC_EINT = (1U << 6),
	WAKE_SRC_CONN_WDT = (1U << 7),
	WAKE_SRC_CCIF0_MD = (1U << 8),
	WAKE_SRC_LOW_BAT = (1U << 9),
	WAKE_SRC_MD32_SPM = (1U << 10),
	WAKE_SRC_F26M_WAKE = (1U << 11),
	WAKE_SRC_F26M_SLEE = (1U << 12),
	WAKE_SRC_PCM_WDT = (1U << 13),
	WAKE_SRC_USB_CD = (1U << 14),
	WAKE_SRC_USB_PDN = (1U << 15),
	WAKE_SRC_C2K_WDT = (1U << 16),
	WAKE_SRC_EINT_SECURE = (1U << 17),
	WAKE_SRC_CCIF1_MD = (1U << 18),
	WAKE_SRC_UART0 = (1U << 19),
	WAKE_SRC_AFE = (1U << 20),
	WAKE_SRC_THERM = (1U << 21),
	WAKE_SRC_CIRQ = (1U << 22),
	WAKE_SRC_MD2_WDT = (1U << 23),
	WAKE_SRC_SYSPWREQ = (1U << 24),
	WAKE_SRC_MD_WDT = (1U << 25),
	WAKE_SRC_CLDMA_MD = (1U << 26),
	WAKE_SRC_SEJ = (1U << 27),
	WAKE_SRC_ALL_MD32 = (1U << 28),
	WAKE_SRC_CPU_IRQ = (1U << 29),
	WAKE_SRC_APSRC_WAKE = (1U << 30),
	WAKE_SRC_APSRC_SLEEP = (1U << 31),
};

typedef enum {
	WR_NONE = 0,
	WR_UART_BUSY = 1,
	WR_PCM_ASSERT = 2,
	WR_PCM_TIMER = 3,
	WR_WAKE_SRC = 4,
	WR_UNKNOWN = 5,
} wake_reason_t;

struct twam_sig {
	u32 sig0;		/* signal 0: config or status */
	u32 sig1;		/* signal 1: config or status */
	u32 sig2;		/* signal 2: config or status */
	u32 sig3;		/* signal 3: config or status */
};

typedef void (*twam_handler_t) (struct twam_sig *twamsig);

/* check if spm firmware ready */
extern int spm_load_firmware_status(void);

/* for power management init */
extern int spm_module_init(void);

/* for ANC in talking */
extern void spm_mainpll_on_request(const char *drv_name);
extern void spm_mainpll_on_unrequest(const char *drv_name);

/* for TWAM in MET */
extern void spm_twam_register_handler(twam_handler_t handler);
extern void spm_twam_enable_monitor(const struct twam_sig *twamsig, bool speed_mode);
extern void spm_twam_disable_monitor(void);
extern void spm_twam_set_idle_select(unsigned int sel);
extern void spm_twam_set_window_length(unsigned int len);
extern void spm_twam_set_mon_type(struct twam_sig *mon);

/* for Vcore DVFS */
extern int spm_go_to_ddrdfs(u32 spm_flags, u32 spm_data);

/* for PMIC power settings */
enum {
	PMIC_PWR_NORMAL = 0,
	PMIC_PWR_DEEPIDLE,
	PMIC_PWR_SODI3,
	PMIC_PWR_SODI,
	PMIC_PWR_SUSPEND,
	PMIC_PWR_NUM,
};
void spm_pmic_power_mode(int mode, int force, int lock);
void spm_bypass_boost_gpio_set(void);

/**************************************
 * Macro and Inline
 **************************************/
#define get_high_cnt(sigsta)		((sigsta) & 0x3ff)
#define get_high_percent(sigsta)	((get_high_cnt(sigsta) * 100 + 511) / 1023)

#endif
