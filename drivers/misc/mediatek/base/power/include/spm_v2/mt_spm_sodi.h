#ifndef __MT_SODI_H_
#define __MT_SODI_H_

#include <mt_cpuidle.h>
#include <mt_spm_idle.h>
#include <mt_spm_misc.h>
#include <mt_spm_internal.h>
#include <mt_spm_pmic_wrap.h>
#include <mt_spm_misc.h>
#include <mt_spm_internal.h>

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#include <mach/mt_secure_api.h>
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#endif

#define WAKE_SRC_FOR_MD32  0

#define reg_read(addr)         __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#ifdef SPM_SODI3_PROFILE_TIME
extern unsigned int	soidle3_profile[4];
#endif

#ifdef SPM_SODI_PROFILE_TIME
extern unsigned int	soidle_profile[4];
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

enum spm_sodi_step {
	SPM_SODI_ENTER = 0,
	SPM_SODI_ENTER_UART_SLEEP,
	SPM_SODI_ENTER_SPM_FLOW,
	SPM_SODI_B3,
	SPM_SODI_B4,
	SPM_SODI_B5,
	SPM_SODI_B6,
	SPM_SODI_ENTER_WFI,
	SPM_SODI_LEAVE_WFI,
	SPM_SODI_LEAVE_SPM_FLOW,
	SPM_SODI_ENTER_UART_AWAKE,
	SPM_SODI_LEAVE,
};

#if SPM_AEE_RR_REC
void __attribute__((weak)) aee_rr_rec_sodi_val(u32 val)
{
}

u32 __attribute__((weak)) aee_rr_curr_sodi_val(void)
{
	return 0;
}

void __attribute__((weak)) aee_rr_rec_sodi3_val(u32 val)
{
	aee_rr_rec_sodi_val(val);
}

u32 __attribute__((weak)) aee_rr_curr_sodi3_val(void)
{
	return aee_rr_curr_sodi_val();
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


static inline void spm_sodi_footprint(enum spm_sodi_step step)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi_val(aee_rr_curr_sodi_val() | (1 << step));
#endif
}

static inline void spm_sodi_footprint_val(u32 val)
{
#if SPM_AEE_RR_REC
		aee_rr_rec_sodi_val(aee_rr_curr_sodi_val() | val);
#endif
}

static inline void spm_sodi_aee_init(void)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi_val(0);
#endif
}

#define spm_sodi_reset_footprint() spm_sodi_aee_init()





#endif /* __MT_SODI_H_ */

