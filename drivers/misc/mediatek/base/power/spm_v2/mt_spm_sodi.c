#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mach/irqs.h>
#include <mach/mt_gpt.h>
#include <mach/mt_secure_api.h>

#include <mt-plat/mt_boot.h>
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mt_cirq.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_io.h>

#include <mt_spm_sodi.h>


/**************************************
 * only for internal debug
 **************************************/

#define SODI_TAG     "[SODI] "
#define sodi_err(fmt, args...)		pr_err(SODI_TAG fmt, ##args)
#define sodi_warn(fmt, args...)		pr_warn(SODI_TAG fmt, ##args)
#define sodi_debug(fmt, args...)	pr_debug(SODI_TAG fmt, ##args)

#define SPM_BYPASS_SYSPWREQ         0

#define LOG_BUF_SIZE					(256)
#define SODI_LOGOUT_TIMEOUT_CRITERIA	(20)
#define SODI_LOGOUT_INTERVAL_CRITERIA	(5000U)	/* unit:ms */


#if defined(CONFIG_OF)
#define MCUCFG_NODE "mediatek,MCUCFG"
static unsigned long mcucfg_base;
static unsigned long mcucfg_phys_base;
#undef MCUCFG_BASE
#define MCUCFG_BASE (mcucfg_base)

#define M4U_NODE "mediatek,M4U"
static unsigned long m4u_base;
static unsigned long m4u_phys_base;
#undef M4U_BASE
#define M4U_BASE (m4u_base)

#else /* #if defined (CONFIG_OF) */
#undef MCUCFG_BASE
#define MCUCFG_BASE 0xF0200000 /* 0x1020_0000 */

#undef M4U_BASE
#define M4U_BASE 0xF0205000 /* 0x1020_5000 */
#endif /* #if defined (CONFIG_OF) */

/* MCUCFG registers */
#define MP0_AXI_CONFIG		(MCUCFG_BASE + 0x2C)
#define MP0_AXI_CONFIG_PHYS	(mcucfg_phys_base + 0x2C)
#define MP1_AXI_CONFIG		(MCUCFG_BASE + 0x22C)
#define MP1_AXI_CONFIG_PHYS	(mcucfg_phys_base + 0x22C)
#define ACINACTM		(1 << 4)

/* M4U registers */
#define MMU_SMI_ASYNC_CFG	(M4U_BASE + 0xB80)
#define MMU_SMI_ASYNC_CFG_PHYS	(m4u_phys_base + 0xB80)
#define SMI_COMMON_ASYNC_DCM	(0x3 << 14)

static struct pwr_ctrl sodi_ctrl = {
	.wake_src = WAKE_SRC_FOR_SODI,

	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
#if defined(CONFIG_ARCH_MT6755)
	.infra_dcm_lock = 1,
#elif defined(CONFIG_ARCH_MT6797)
	.infra_dcm_lock = 0, /* set to be 0 if SODI original */
#endif
	.wfi_op = WFI_OP_AND,

	/* SPM_AP_STANDBY_CON */
	.mp0top_idle_mask = 0,
	.mp1top_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.md_ddr_dbc_en = 0,
	.md1_req_mask_b = 1,
	.md2_req_mask_b = 0, /* bit 20 */
#if defined(CONFIG_ARCH_MT6755)
	.scp_req_mask_b = 0, /* bit 21 */
#elif defined(CONFIG_ARCH_MT6797)
	.scp_req_mask_b = 1, /* bit 21 */
#endif
	.lte_mask_b = 0,
	.md_apsrc1_sel = 0, /* bit 24 */
	.md_apsrc0_sel = 0, /* bit 25 */
	.conn_mask_b = 1,
	.conn_apsrc_sel = 0, /* bit 27 */

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_lte_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 0,
	.spm_ddren_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_SRC_MASK */
	.ccif0_to_md_mask_b = 1,
	.ccif0_to_ap_mask_b = 1,
	.ccif1_to_md_mask_b = 1,
	.ccif1_to_ap_mask_b = 1,
	.ccifmd_md1_event_mask_b = 1,
	.ccifmd_md2_event_mask_b = 1,
	.vsync_mask_b = 0,	/* 5-bit */
	.md_srcclkena_0_infra_mask_b = 0, /* bit 12 */
	.md_srcclkena_1_infra_mask_b = 0, /* bit 13 */
	.conn_srcclkena_infra_mask_b = 0, /* bit 14 */
	.md32_srcclkena_infra_mask_b = 0, /* bit 15 */
	.srcclkeni_infra_mask_b = 0, /* bit 16 */
	.md_apsrcreq_0_infra_mask_b = 1,
	.md_apsrcreq_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 1,
#if defined(CONFIG_ARCH_MT6755)
	.md32_apsrcreq_infra_mask_b = 0,
#elif defined(CONFIG_ARCH_MT6797)
	.md32_apsrcreq_infra_mask_b = 1,
#endif
	.md_ddr_en_0_mask_b = 1,
	.md_ddr_en_1_mask_b = 0, /* bit 22 */
	.md_vrf18_req_0_mask_b = 1,
	.md_vrf18_req_1_mask_b = 0, /* bit 24 */
	.emi_bw_dvfs_req_mask = 1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0x1f, /* 5-bit */
	.vdec_req_mask_b = 0, /* bit 6 */
	.gce_req_mask_b = 1, /* bit 7, set to be 1 for SODI */
	.cpu_md_dvfs_erq_merge_mask_b = 0,
	.md1_ddr_en_dvfs_halt_mask_b = 0,
	.md2_ddr_en_dvfs_halt_mask_b = 0,
	.vsync_dvfs_halt_mask_b = 0, /* 5-bit, bit 11 ~ 15 */
	.conn_ddr_en_mask_b = 1,
	.disp_req_mask_b = 1, /* bit 17, set to be 1 for SODI */
	.disp1_req_mask_b = 1, /* bit 18, set to be 1 for SODI */
#if defined(CONFIG_ARCH_MT6755)
	.mfg_req_mask_b = 0, /* bit 19 */
#elif defined(CONFIG_ARCH_MT6797)
	.mfg_req_mask_b = 1, /* bit 19, set to be 1 for SODI */
#endif
	.c2k_ps_rccif_wake_mask_b = 1,
	.c2k_l1_rccif_wake_mask_b = 1,
	.ps_c2k_rccif_wake_mask_b = 1,
	.l1_c2k_rccif_wake_mask_b = 1,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
#if defined(CONFIG_ARCH_MT6797)
	.disp_od_req_mask_b = 1, /* bit 27, set to be 1 for SODI */
#endif
	/* SPM_CLK_CON */
	.srclkenai_mask = 1,

	.mp1_cpu0_wfi_en	= 1,
	.mp1_cpu1_wfi_en	= 1,
	.mp1_cpu2_wfi_en	= 1,
	.mp1_cpu3_wfi_en	= 1,
	.mp0_cpu0_wfi_en	= 1,
	.mp0_cpu1_wfi_en	= 1,
	.mp0_cpu2_wfi_en	= 1,
	.mp0_cpu3_wfi_en	= 1,

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 1,
#endif
};

/* please put firmware to vendor/mediatek/proprietary/hardware/spm/mtxxxx/ */
struct spm_lp_scen __spm_sodi = {
	.pwrctrl = &sodi_ctrl,
};

/* 0:power-down mode, 1:CG mode */
static bool gSpm_SODI_mempll_pwr_mode;
static bool gSpm_sodi_en;

static unsigned long int sodi_logout_prev_time;
static int pre_emi_refresh_cnt;
static int memPllCG_prev_status = 1;	/* 1:CG, 0:pwrdn */
static unsigned int logout_sodi_cnt;
static unsigned int logout_selfrefresh_cnt;

void spm_trigger_wfi_for_sodi(struct pwr_ctrl *pwrctrl)
{
	u32 v0, v1;

	if (is_cpu_pdn(pwrctrl->pcm_flags)) {
		mt_cpu_dormant(CPU_SODI_MODE);
	} else {
		/* backup MPx_AXI_CONFIG */
		v0 = reg_read(MP0_AXI_CONFIG);
		v1 = reg_read(MP1_AXI_CONFIG);

		/* disable snoop function */
		MCUSYS_SMC_WRITE(MP0_AXI_CONFIG, v0 | ACINACTM);
		MCUSYS_SMC_WRITE(MP1_AXI_CONFIG, v1 | ACINACTM);

		sodi_debug("enter legacy WFI, MP0_AXI_CONFIG=0x%x, MP1_AXI_CONFIG=0x%x\n",
			   reg_read(MP0_AXI_CONFIG), reg_read(MP1_AXI_CONFIG));

		/* enter WFI */
		wfi_with_sync();

		/* restore MP0_AXI_CONFIG */
		MCUSYS_SMC_WRITE(MP0_AXI_CONFIG, v0);
		MCUSYS_SMC_WRITE(MP1_AXI_CONFIG, v1);

		sodi_debug("exit legacy WFI, MP0_AXI_CONFIG=0x%x, MP1_AXI_CONFIG=0x%x\n",
			   reg_read(MP0_AXI_CONFIG), reg_read(MP1_AXI_CONFIG));
	}
}

static u32 mmu_smi_async_cfg;
void spm_disable_mmu_smi_async(void)
{
	mmu_smi_async_cfg = reg_read(MMU_SMI_ASYNC_CFG);
	reg_write(MMU_SMI_ASYNC_CFG, mmu_smi_async_cfg | SMI_COMMON_ASYNC_DCM);
}

void spm_enable_mmu_smi_async(void)
{
	reg_write(MMU_SMI_ASYNC_CFG, mmu_smi_async_cfg);
}


static void spm_sodi_pre_process(void)
{
#if defined(CONFIG_ARCH_MT6755)
	u32 val;
#endif

	__spm_pmic_pg_force_on();
	spm_disable_mmu_smi_async();
	spm_bypass_boost_gpio_set();

#if defined(CONFIG_ARCH_MT6755)
	pmic_read_interface_nolock(MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_ADDR,
					&val,
					MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_MASK,
					MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_SHIFT);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_NORMAL, val);

	pmic_read_interface_nolock(MT6351_TOP_CON, &val, 0x037F, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
					IDX_DI_SRCCLKEN_IN2_NORMAL,
					val | (1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
					IDX_DI_SRCCLKEN_IN2_SLEEP,
					val & ~(1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));
#endif

	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_DEEPIDLE);

	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off())
		__spm_bsi_top_init_setting();
}

static void spm_sodi_post_process(void)
{
	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);

	spm_enable_mmu_smi_async();
	__spm_pmic_pg_force_off();
}


static wake_reason_t
spm_sodi_output_log(struct wake_status *wakesta, struct pcm_desc *pcmdesc, int vcore_status, u32 sodi_flags)
{
	wake_reason_t wr = WR_NONE;
	unsigned long int sodi_logout_curr_time = 0;
	int need_log_out = 0;

	if (sodi_flags&SODI_FLAG_NO_LOG) {
		if (wakesta->assert_pc != 0) {
			sodi_err("PCM ASSERT AT %u (%s), r13 = 0x%x, debug_flag = 0x%x\n",
					wakesta->assert_pc, pcmdesc->version, wakesta->r13, wakesta->debug_flag);
			wr = WR_PCM_ASSERT;
		}
	} else if (!(sodi_flags&SODI_FLAG_REDUCE_LOG) || (sodi_flags & SODI_FLAG_RESIDENCY)) {
		sodi_warn("vcore_status = %d, self_refresh = 0x%x, sw_flag = 0x%x, 0x%x, %s\n",
				vcore_status, spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
				spm_read(DUMMY1_PWR_CON), pcmdesc->version);
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false);
	} else {
		sodi_logout_curr_time = spm_get_current_time_ms();

		if (wakesta->assert_pc != 0) {
			need_log_out = 1;
		} else if ((wakesta->r12 & (0x1 << 4)) == 0) {
			/* not wakeup by GPT */
			need_log_out = 1;
		} else if (wakesta->timer_out <= SODI_LOGOUT_TIMEOUT_CRITERIA) {
			need_log_out = 1;
		} else if ((spm_read(SPM_PASR_DPD_0) == 0 && pre_emi_refresh_cnt > 0) ||
				(spm_read(SPM_PASR_DPD_0) > 0 && pre_emi_refresh_cnt == 0)) {
			need_log_out = 1;
		} else if ((sodi_logout_curr_time - sodi_logout_prev_time) > SODI_LOGOUT_INTERVAL_CRITERIA) {
			/* previous logout time > SODI_LOGOUT_INTERVAL_CRITERIA */
			need_log_out = 1;
		} else {
			/* check CG/pwrdn status is changed */
			int mem_status = 0;

			/* check mempll CG/pwrdn status change */
			if (((spm_read(SPM_SW_FLAG) & SPM_FLAG_SODI_CG_MODE) != 0) ||
				((spm_read(DUMMY1_PWR_CON) & DUMMY1_PWR_ISO_LSB) != 0))
				mem_status = 1;

			if (memPllCG_prev_status != mem_status) {
				memPllCG_prev_status = mem_status;
				need_log_out = 1;
			}
		}

		logout_sodi_cnt++;
		logout_selfrefresh_cnt += spm_read(SPM_PASR_DPD_0);
		pre_emi_refresh_cnt = spm_read(SPM_PASR_DPD_0);

		if (need_log_out == 1) {
			sodi_logout_prev_time = sodi_logout_curr_time;

			if (wakesta->assert_pc != 0) {
				sodi_err("wake up by SPM assert, vcore_status = %d, self_refresh = 0x%x, sw_flag = 0x%x, 0x%x, %s\n",
						vcore_status, spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON), pcmdesc->version);

				sodi_err("sodi_cnt = %d, self_refresh_cnt = 0x%x, spm_pc = 0x%0x, r13 = 0x%x, debug_flag = 0x%x\n",
						logout_sodi_cnt, logout_selfrefresh_cnt,
						wakesta->assert_pc, wakesta->r13, wakesta->debug_flag);

				sodi_err("r12 = 0x%x, r12_e = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
						wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
						wakesta->event_reg, wakesta->isr);

			} else {
				char buf[LOG_BUF_SIZE] = { 0 };
				int i;

				if (wakesta->r12 & WAKE_SRC_R12_PCM_TIMER) {
					if (wakesta->wake_misc & WAKE_MISC_PCM_TIMER)
						strcat(buf, " PCM_TIMER");

					if (wakesta->wake_misc & WAKE_MISC_TWAM)
						strcat(buf, " TWAM");

					if (wakesta->wake_misc & WAKE_MISC_CPU_WAKE)
						strcat(buf, " CPU");
				}
				for (i = 1; i < 32; i++) {
					if (wakesta->r12 & (1U << i)) {
						strcat(buf, wakesrc_str[i]);
						wr = WR_WAKE_SRC;
					}
				}
				BUG_ON(strlen(buf) >= LOG_BUF_SIZE);

				sodi_warn("wake up by %s, vcore_status = %d, self_refresh = 0x%x, sw_flag = 0x%x, 0x%x, %s\n",
						buf, vcore_status, spm_read(SPM_PASR_DPD_0), spm_read(SPM_SW_FLAG),
						spm_read(DUMMY1_PWR_CON), pcmdesc->version);

				sodi_warn("sodi_cnt = %d, self_refresh_cnt = 0x%x, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x\n",
						logout_sodi_cnt, logout_selfrefresh_cnt,
						wakesta->timer_out, wakesta->r13, wakesta->debug_flag);

				sodi_warn("r12 = 0x%x, r12_e = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
						wakesta->r12, wakesta->r12_ext, wakesta->raw_sta, wakesta->idle_sta,
						wakesta->event_reg, wakesta->isr);

			}

			logout_sodi_cnt = 0;
			logout_selfrefresh_cnt = 0;
		}
	}
	return wr;
}
wake_reason_t spm_go_to_sodi(u32 spm_flags, u32 spm_data, u32 sodi_flags)
{
	struct wake_status wakesta;
	unsigned long flags;
	struct mtk_irq_mask *mask;
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_sodi.pwrctrl;
	int vcore_status = vcorefs_get_curr_ddr();
	u32 cpu = spm_data;
	u32 sodi_idx;

#if defined(CONFIG_ARCH_MT6797)
	sodi_idx = spm_get_sodi_pcm_index() + cpu / 4;
#else
	sodi_idx = DYNA_LOAD_PCM_SODI + cpu / 4;
#endif

	if (!dyna_load_pcm[sodi_idx].ready) {
		sodi_err("error: load firmware fail\n");
		BUG();
	}
	pcmdesc = &(dyna_load_pcm[sodi_idx].desc);


	spm_sodi_footprint(SPM_SODI_ENTER);

	if (gSpm_SODI_mempll_pwr_mode == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE;	/* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE;	/* PDN mode */

	update_pwrctrl_pcm_flags(&spm_flags);
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	/* enable APxGPT timer */
	soidle_before_wfi(cpu);

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);

	mask = kmalloc(sizeof(struct mtk_irq_mask), GFP_ATOMIC);
	if (!mask) {
		wr = -ENOMEM;
		goto UNLOCK_SPM;
	}
	mt_irq_mask_all(mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_sodi_footprint(SPM_SODI_ENTER_UART_SLEEP);

	if (request_uart_to_sleep()) {
		wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}

	spm_sodi_footprint(SPM_SODI_ENTER_SPM_FLOW);

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_check_md_pdn_power_control(pwrctrl);

	__spm_sync_vcore_dvfs_power_control(pwrctrl, __spm_vcore_dvfs.pwrctrl);

#if defined(CONFIG_ARCH_MT6797)
	if (spm_read(SPM_SW_FLAG) & SPM_FLAG_SODI_CG_MODE) {
		/* the following masks set to be 1 only for SODI CG mode */
		pwrctrl->md_apsrc1_sel = 1;
		pwrctrl->md_apsrc0_sel = 1;
		pwrctrl->conn_apsrc_sel = 1;
	} else {
		/* the following masks set to be 0 which dynamic switch by FW */
		pwrctrl->md_apsrc1_sel = 0;
		pwrctrl->md_apsrc0_sel = 0;
		pwrctrl->conn_apsrc_sel = 0;
	}
#endif

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	if (pwrctrl->timer_val_cust == 0) {
		if (spm_read(PCM_TIMER_VAL) > PCM_TIMER_MAX)
			spm_write(PCM_TIMER_VAL, PCM_TIMER_MAX);
		spm_write(PCM_CON1, spm_read(PCM_CON1) | SPM_REGWR_CFG_KEY | PCM_TIMER_EN_LSB);
	}

	spm_sodi_pre_process();

	__spm_kick_pcm_to_run(pwrctrl);


	spm_sodi_footprint_val((1 << SPM_SODI_ENTER_WFI) |
				(1 << SPM_SODI_B3) | (1 << SPM_SODI_B4) |
				(1 << SPM_SODI_B5) | (1 << SPM_SODI_B6));


#ifdef SPM_SODI_PROFILE_TIME
	gpt_get_cnt(SPM_SODI_PROFILE_APXGPT, &soidle_profile[1]);
#endif

	spm_trigger_wfi_for_sodi(pwrctrl);

#ifdef SPM_SODI_PROFILE_TIME
	gpt_get_cnt(SPM_SODI_PROFILE_APXGPT, &soidle_profile[2]);
#endif


	spm_sodi_footprint(SPM_SODI_LEAVE_WFI);

	spm_sodi_post_process();

	__spm_get_wakeup_status(&wakesta);

	__spm_clean_after_wakeup();

	spm_sodi_footprint(SPM_SODI_ENTER_UART_AWAKE);

	request_uart_to_wakeup();

	wr = spm_sodi_output_log(&wakesta, pcmdesc, vcore_status, sodi_flags);

	spm_sodi_footprint(SPM_SODI_LEAVE_SPM_FLOW);

RESTORE_IRQ:
#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif
	mt_irq_mask_restore(mask);
	kfree(mask);

UNLOCK_SPM:
	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();

	/* stop APxGPT timer and enable caore0 local timer */
	soidle_after_wfi(cpu);

	spm_sodi_reset_footprint();

	return wr;
}

void spm_sodi_mempll_pwr_mode(bool pwr_mode)
{
	gSpm_SODI_mempll_pwr_mode = pwr_mode;
}

bool spm_get_sodi_mempll(void)
{
	return gSpm_SODI_mempll_pwr_mode;
}

void spm_enable_sodi(bool en)
{
	gSpm_sodi_en = en;
}

bool spm_get_sodi_en(void)
{
	return gSpm_sodi_en;
}

void spm_sodi_init(void)
{
#if defined(CONFIG_OF)
	struct device_node *node;
	struct resource r;

	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		sodi_err("error: cannot find node " MCUCFG_NODE);
		goto mcucfg_exit;
	}
	if (of_address_to_resource(node, 0, &r)) {
		sodi_err("error: cannot get phys addr" MCUCFG_NODE);
		goto mcucfg_exit;
	}
	mcucfg_phys_base = r.start;

	mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!mcucfg_base) {
		sodi_err("error: cannot iomap " MCUCFG_NODE);
		goto mcucfg_exit;
	}

	sodi_debug("mcucfg_base = 0x%u\n", (unsigned int)mcucfg_base);

mcucfg_exit:
	/* m4u */
	node = of_find_compatible_node(NULL, NULL, M4U_NODE);
	if (!node) {
		sodi_err("error: cannot find node " M4U_NODE);
		goto m4u_exit;
	}
	if (of_address_to_resource(node, 0, &r)) {
		sodi_err("error: cannot get phys addr" M4U_NODE);
		goto m4u_exit;
	}
	m4u_phys_base = r.start;

	m4u_base = (unsigned long)of_iomap(node, 0);
	if (!m4u_base) {
		sodi_err("error: cannot iomap " M4U_NODE);
		goto m4u_exit;
	}

	sodi_debug("m4u_base = 0x%u\n", (unsigned int)m4u_base);

m4u_exit:
	sodi_debug("spm_sodi_init\n");
#endif

	spm_sodi_aee_init();
}

MODULE_DESCRIPTION("SPM-SODI Driver v0.1");
