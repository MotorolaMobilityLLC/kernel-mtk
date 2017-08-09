#ifndef __MT_SPM_SODI3_H_
#define __MT_SPM_SODI3_H_

#include <mt_cpuidle.h>
#include <mt_spm_idle.h>
#include <mt_spm_misc.h>
#include <mt_spm_internal.h>
#include <mt_spm_pmic_wrap.h>
#include <mt_spm_misc.h>
#include <mt_spm_internal.h>

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#endif

#if defined(CONFIG_ARCH_MT6755)

#define WAKE_SRC_FOR_SODI3 \
	(WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_APXGPT1_EVENT_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_SYS_CIRQ_IRQ_B | \
	WAKE_SRC_R12_CSYSPWREQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_SEJ_WDT_GPT_B)
#define WAKE_SRC_FOR_MD32  0

#elif defined(CONFIG_ARCH_MT6797)

#define WAKE_SRC_FOR_SODI3 \
	(WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_APXGPT1_EVENT_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_USB0_CDSC_B_AND_USB1_CSDC_B | \
	WAKE_SRC_R12_USB0_POWERDWN_B_AND_USB1_POWERDWN_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_SYS_CIRQ_IRQ_B | \
	WAKE_SRC_R12_CSYSPWREQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_SEJ_WDT_B_AND_SEJ_GPT_B)
#define WAKE_SRC_FOR_MD32  0

#else
#error "Does not support!"
#endif


#define reg_read(addr)         __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#ifdef SPM_SODI3_PROFILE_TIME
extern unsigned int	soidle3_profile[4];
#endif

enum spm_sodi3_step {
	SPM_SODI3_ENTER = 0,
	SPM_SODI3_ENTER_UART_SLEEP,
	SPM_SODI3_ENTER_SPM_FLOW,
	SPM_SODI3_B3,
	SPM_SODI3_B4,
	SPM_SODI3_B5,
	SPM_SODI3_B6,
	SPM_SODI3_ENTER_WFI,
	SPM_SODI3_LEAVE_WFI,
	SPM_SODI3_LEAVE_SPM_FLOW,
	SPM_SODI3_ENTER_UART_AWAKE,
	SPM_SODI3_LEAVE,
};


#if SPM_AEE_RR_REC
void __attribute__((weak)) aee_rr_rec_sodi3_val(u32 val)
{
}

u32 __attribute__((weak)) aee_rr_curr_sodi3_val(void)
{
	return 0;
}

#endif

static inline void spm_sodi3_footprint(enum spm_sodi3_step step)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi3_val(aee_rr_curr_sodi3_val() | (1 << step));
#endif
}

static inline void spm_sodi3_footprint_val(u32 val)
{
#if SPM_AEE_RR_REC
		aee_rr_rec_sodi3_val(aee_rr_curr_sodi3_val() | val);
#endif
}

static inline void spm_sodi3_aee_init(void)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi3_val(0);
#endif
}

#define spm_sodi3_reset_footprint() spm_sodi3_aee_init()


extern void spm_trigger_wfi_for_sodi(struct pwr_ctrl *pwrctrl);
extern void spm_enable_mmu_smi_async(void);
extern void spm_disable_mmu_smi_async(void);

#endif /* __MT_SPM_SODI3_H_ */

