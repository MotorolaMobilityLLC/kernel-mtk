#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include "mt_vcore_dvfs.h"
#include "mt_cpufreq.h"
#include "mt_spm_internal.h"

#define PER_OPP_DVS_US		600

#define VCORE_STA_UHPM		(VCORE_STA_1 | VCORE_STA_0)
#define VCORE_STA_HPM		VCORE_STA_1
#define VCORE_STA_LPM		0

#define get_vcore_sta()		(spm_read(SPM_SLEEP_DVFS_STA) & (VCORE_STA_1 | VCORE_STA_0))

static const u32 vcore_dvfs_binary[] = {
	0x1840001f, 0x00000001, 0x1990001f, 0x10006b08, 0xe8208000, 0x10006b18,
	0x00000000, 0x1910001f, 0x100062c4, 0x1a00001f, 0x10006b0c, 0x1950001f,
	0x10006b0c, 0x80849001, 0xa1508805, 0xe2000005, 0x1910001f, 0x100062c4,
	0x10c0041f, 0x80801001, 0x81600801, 0xa0df1403, 0xa0df8803, 0xd8000402,
	0x17c07c1f, 0x80809001, 0x81600801, 0xa0d59403, 0xa0d60803, 0xd8000402,
	0x17c07c1f, 0x80ff0403, 0xd8000525, 0x17c07c1f, 0x80841001, 0x81600801,
	0xa0db9403, 0xa0de0803, 0xd8000525, 0x17c07c1f, 0x80f60403, 0x13000c1f,
	0xe8208000, 0x10006310, 0x0b1603f8, 0x1b80001f, 0x90100000, 0x88900001,
	0x10006814, 0xd8200202, 0x17c07c1f, 0x18d0001f, 0x10006b6c, 0x78a00003,
	0x0000beef, 0xd80007a2, 0x17c07c1f, 0xc0c02d40, 0x17c07c1f, 0xd0000200,
	0x17c07c1f, 0x1910001f, 0x10006b0c, 0x1a00001f, 0x100062c4, 0x1950001f,
	0x100062c4, 0x80809001, 0x81748405, 0xa1548805, 0xe2000005, 0x80841401,
	0xd8000982, 0x8204b401, 0xc8c00a68, 0x17c07c1f, 0x1ac0001f, 0x55aa55aa,
	0x1940001f, 0xaa55aa55, 0x1b80001f, 0x00001000, 0xf0000000, 0x81429801,
	0xd8000c45, 0x17c07c1f, 0x1a00001f, 0x10006604, 0x18c0001f, 0x10001124,
	0x1910001f, 0x10001124, 0xa1108404, 0xe0c00004, 0x1910001f, 0x10001124,
	0xc0c025a0, 0xe2200007, 0x1a00001f, 0x100062c4, 0x1890001f, 0x100062c4,
	0xa1540402, 0xe2000005, 0x1b00001f, 0x50000001, 0x80800402, 0xd8200de2,
	0x17c07c1f, 0x1b00001f, 0x90000001, 0xf0000000, 0x17c07c1f, 0x81429801,
	0xd8001005, 0x17c07c1f, 0x1a00001f, 0x10006604, 0x18c0001f, 0x10001124,
	0x1910001f, 0x10001124, 0x81308404, 0xe0c00004, 0x1910001f, 0x10001124,
	0xc0c025a0, 0xe2200007, 0x1a00001f, 0x100062c4, 0x1890001f, 0x100062c4,
	0x81740402, 0xe2000005, 0x1b00001f, 0x40801001, 0x80800402, 0xd82011a2,
	0x17c07c1f, 0x1b00001f, 0x80800001, 0xf0000000, 0x17c07c1f, 0x81441801,
	0xd8201405, 0x17c07c1f, 0x1a00001f, 0x10006604, 0xc0c025a0, 0xe2200004,
	0xc0c02680, 0x1093041f, 0xc0c025a0, 0xe2200003, 0xc0c02680, 0x1093041f,
	0xc0c025a0, 0xe2200002, 0xc0c02680, 0x1093041f, 0x1a00001f, 0x100062c4,
	0x1890001f, 0x100062c4, 0xa0908402, 0xe2000002, 0x1b00001f, 0x40801001,
	0xf0000000, 0x17c07c1f, 0x81441801, 0xd8201765, 0x17c07c1f, 0x1a00001f,
	0x10006604, 0xc0c025a0, 0xe2200003, 0xc0c02680, 0x1093041f, 0xc0c025a0,
	0xe2200004, 0xc0c02680, 0x1093041f, 0xc0c025a0, 0xe2200005, 0xc0c02680,
	0x1093041f, 0x1a00001f, 0x100062c4, 0x1890001f, 0x100062c4, 0x80b08402,
	0xe2000002, 0x1b00001f, 0x00000801, 0xf0000000, 0x17c07c1f, 0x1890001f,
	0x10006b0c, 0x81400402, 0xd8001a25, 0x17c07c1f, 0x18d0001f, 0x100063e8,
	0x81003001, 0xd8001ec4, 0x1300041f, 0xd0001ec0, 0x13000c1f, 0x81441801,
	0xd8201c45, 0x17c07c1f, 0x1a00001f, 0x10006604, 0xc0c025a0, 0xe2200001,
	0xc0c02680, 0x1093041f, 0xc0c025a0, 0xe2200000, 0xc0c02680, 0x1093041f,
	0xc0c025a0, 0xe2200006, 0xc0c02680, 0x1093041f, 0x1880001f, 0x10006608,
	0x18d0001f, 0x10006608, 0x80f98403, 0x80fb8403, 0xe0800003, 0x1a00001f,
	0x100062c4, 0x1890001f, 0x100062c4, 0xa1400402, 0xe2000005, 0x1b00001f,
	0x90000001, 0x80840801, 0xd8001ec2, 0x17c07c1f, 0x1b00001f, 0x80800001,
	0xf0000000, 0x17c07c1f, 0x1a00001f, 0x100062c4, 0x1890001f, 0x100062c4,
	0x81400402, 0xd80020c5, 0x17c07c1f, 0x18d0001f, 0x100063e8, 0x81003001,
	0xd8002564, 0x1300041f, 0xd0002560, 0x13000c1f, 0x81441801, 0xd82022e5,
	0x17c07c1f, 0x1a00001f, 0x10006604, 0xc0c025a0, 0xe2200000, 0xc0c02680,
	0x1093041f, 0xc0c025a0, 0xe2200001, 0xc0c02680, 0x1093041f, 0xc0c025a0,
	0xe2200002, 0xc0c02680, 0x1093041f, 0x1880001f, 0x10006608, 0x18d0001f,
	0x10006608, 0xa0d98403, 0xa0db8403, 0xe0800003, 0x1a00001f, 0x100062c4,
	0x1890001f, 0x100062c4, 0x81600402, 0xe2000005, 0x1b00001f, 0x50000001,
	0x80840801, 0xd8002562, 0x17c07c1f, 0x1b00001f, 0x40801001, 0xf0000000,
	0x17c07c1f, 0x18d0001f, 0x10006604, 0x10cf8c1f, 0xd82025a3, 0x17c07c1f,
	0xf0000000, 0x17c07c1f, 0x81499801, 0xd82027e5, 0x17c07c1f, 0xd8202d02,
	0x17c07c1f, 0x18d0001f, 0x40000000, 0x18d0001f, 0x70000000, 0xd80026e2,
	0x00a00402, 0x814a1801, 0xd8202945, 0x17c07c1f, 0xd8202d02, 0x17c07c1f,
	0x18d0001f, 0x40000000, 0x18d0001f, 0x80000000, 0xd8002842, 0x00a00402,
	0x814a9801, 0xd8202aa5, 0x17c07c1f, 0xd8202d02, 0x17c07c1f, 0x18d0001f,
	0x40000000, 0x18d0001f, 0xc0000000, 0xd80029a2, 0x00a00402, 0x814c1801,
	0xd8202c05, 0x17c07c1f, 0xd8202d02, 0x17c07c1f, 0x18d0001f, 0x40000000,
	0x18d0001f, 0xa0000000, 0xd8002b02, 0x00a00402, 0xd8202d02, 0x17c07c1f,
	0x18d0001f, 0x40000000, 0x18d0001f, 0x40000000, 0xd8002c02, 0x00a00402,
	0xf0000000, 0x17c07c1f, 0x1900001f, 0x10006014, 0x1950001f, 0x10006014,
	0xa1508405, 0xe1000005, 0x1900001f, 0x10006814, 0xe100001f, 0x812ab401,
	0xd8002e64, 0x17c07c1f, 0x1880001f, 0x10006284, 0x18d0001f, 0x10006284,
	0x80f20403, 0xe0800003, 0x80f08403, 0xe0800003, 0x1900001f, 0x10006014,
	0x1950001f, 0x10006014, 0x81708405, 0xe1000005, 0x1900001f, 0x10006b6c,
	0xe100001f, 0xf0000000, 0x17c07c1f
};
static struct pcm_desc vcore_dvfs_pcm = {
	.version	= "pcm_vcore_dvfs_v0.5.8_20150904-slow_dvs",
	.base		= vcore_dvfs_binary,
	.size		= 393,
	.sess		= 1,
	.replace	= 1,
	.vec0		= EVENT_VEC(23, 1, 0, 83),	/* FUNC_MD_VRF18_WAKEUP */
	.vec1		= EVENT_VEC(28, 1, 0, 113),	/* FUNC_MD_VRF18_SLEEP */
	.vec2		= EVENT_VEC(11, 1, 0, 143),	/* FUNC_26M_WAKEUP */
	.vec3		= EVENT_VEC(12, 1, 0, 170),	/* FUNC_26M_SLEEP */
	.vec4		= EVENT_VEC(30, 1, 0, 197),	/* FUNC_UHPM_WAKEUP */
	.vec5		= EVENT_VEC(31, 1, 0, 248),	/* FUNC_UHPM_SLEEP */
};

static struct pwr_ctrl vcore_dvfs_ctrl = {
	.md1_req_mask		= 0,
	.conn_mask		= 0,
	.md_vrf18_req_mask_b	= 0,

	/* mask to avoid affecting apsrc_state */
	.md2_req_mask		= 1,
	.ccif0_to_ap_mask	= 1,
	.ccif0_to_md_mask	= 1,
	.ccif1_to_ap_mask	= 1,
	.ccif1_to_md_mask	= 1,
	.ccifmd_md1_event_mask	= 1,
	.ccifmd_md2_event_mask	= 1,
	.gce_req_mask		= 1,
	.disp_req_mask		= 1,
	.mfg_req_mask		= 1,
};

struct spm_lp_scen __spm_vcore_dvfs = {
	.pcmdesc	= &vcore_dvfs_pcm,
	.pwrctrl	= &vcore_dvfs_ctrl,
};

char *spm_dump_vcore_dvs_regs(char *p)
{
	if (p) {
		p += sprintf(p, "SLEEP_DVFS_STA: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA));
		p += sprintf(p, "PCM_SRC_REQ   : 0x%x\n", spm_read(SPM_PCM_SRC_REQ));
		p += sprintf(p, "PCM_REG13_DATA: 0x%x\n", spm_read(SPM_PCM_REG13_DATA));
		p += sprintf(p, "PCM_REG12_DATA: 0x%x\n", spm_read(SPM_PCM_REG12_DATA));
		p += sprintf(p, "PCM_REG12_MASK: 0x%x\n", spm_read(SPM_PCM_REG12_MASK));
		p += sprintf(p, "AP_STANBY_CON : 0x%x\n", spm_read(SPM_AP_STANBY_CON));
		p += sprintf(p, "PCM_IM_PTR    : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR), spm_read(SPM_PCM_IM_LEN));
		p += sprintf(p, "PCM_REG6_DATA : 0x%x\n", spm_read(SPM_PCM_REG6_DATA));
		p += sprintf(p, "PCM_REG11_DATA: 0x%x\n", spm_read(SPM_PCM_REG11_DATA));
	} else {
		spm_err("[VcoreFS] SLEEP_DVFS_STA: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA));
		spm_err("[VcoreFS] PCM_SRC_REQ   : 0x%x\n", spm_read(SPM_PCM_SRC_REQ));
		spm_err("[VcoreFS] PCM_REG13_DATA: 0x%x\n", spm_read(SPM_PCM_REG13_DATA));
		spm_err("[VcoreFS] PCM_REG12_DATA: 0x%x\n", spm_read(SPM_PCM_REG12_DATA));
		spm_err("[VcoreFS] PCM_REG12_MASK: 0x%x\n", spm_read(SPM_PCM_REG12_MASK));
		spm_err("[VcoreFS] AP_STANBY_CON : 0x%x\n", spm_read(SPM_AP_STANBY_CON));
		spm_err("[VcoreFS] PCM_IM_PTR    : 0x%x (%u)\n", spm_read(SPM_PCM_IM_PTR), spm_read(SPM_PCM_IM_LEN));
		spm_err("[VcoreFS] PCM_REG6_DATA : 0x%x\n", spm_read(SPM_PCM_REG6_DATA));
		spm_err("[VcoreFS] PCM_REG11_DATA: 0x%x\n", spm_read(SPM_PCM_REG11_DATA));

		spm_err("[VcoreFS] PCM_REG0_DATA : 0x%x\n", spm_read(SPM_PCM_REG0_DATA));
		spm_err("[VcoreFS] PCM_REG1_DATA : 0x%x\n", spm_read(SPM_PCM_REG1_DATA));
		spm_err("[VcoreFS] PCM_REG2_DATA : 0x%x\n", spm_read(SPM_PCM_REG2_DATA));
		spm_err("[VcoreFS] PCM_REG3_DATA : 0x%x\n", spm_read(SPM_PCM_REG3_DATA));
		spm_err("[VcoreFS] PCM_REG4_DATA : 0x%x\n", spm_read(SPM_PCM_REG4_DATA));
		spm_err("[VcoreFS] PCM_REG5_DATA : 0x%x\n", spm_read(SPM_PCM_REG5_DATA));
		spm_err("[VcoreFS] PCM_REG7_DATA : 0x%x\n", spm_read(SPM_PCM_REG7_DATA));
		spm_err("[VcoreFS] PCM_REG8_DATA : 0x%x\n", spm_read(SPM_PCM_REG8_DATA));
		spm_err("[VcoreFS] PCM_REG9_DATA : 0x%x\n", spm_read(SPM_PCM_REG9_DATA));
		spm_err("[VcoreFS] PCM_REG10_DATA: 0x%x\n", spm_read(SPM_PCM_REG10_DATA));
		spm_err("[VcoreFS] PCM_REG14_DATA: 0x%x\n", spm_read(SPM_PCM_REG14_DATA));
		spm_err("[VcoreFS] PCM_REG15_DATA: %u\n"  , spm_read(SPM_PCM_REG15_DATA));
	}

	return p;
}

static void __go_to_vcore_dvfs(u32 spm_flags, u8 f26m_req, u8 apsrc_req)
{
	struct pcm_desc *pcmdesc = __spm_vcore_dvfs.pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_vcore_dvfs.pwrctrl;

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags /*| SPM_MD_VRF18_DIS*/);
	pwrctrl->pcm_f26m_req = f26m_req;
	pwrctrl->pcm_apsrc_req = apsrc_req;

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);
}

#define wait_pcm_complete_dvs(condition, timeout)	\
({							\
	int i = 0;					\
	while (!(condition)) {				\
		if (i >= (timeout)) {			\
			i = -EBUSY;			\
			break;				\
		}					\
		udelay(1);				\
		i++;					\
	}						\
	i;						\
})

static bool is_fw_not_existed(void)
{
	return spm_read(SPM_PCM_REG1_DATA) != 0x1 || spm_read(SPM_PCM_REG11_DATA) == 0x55AA55AA;
}

static bool is_fw_not_support_uhpm(void)
{
	u32 pcm_ptr = spm_read(SPM_PCM_IM_PTR);
	u32 vcore_ptr = base_va_to_pa(__spm_vcore_dvfs.pcmdesc->base);
	/*u32 sodi_ptr = base_va_to_pa(__spm_sodi.pcmdesc->base);*/

	return pcm_ptr != vcore_ptr /*&& pcm_ptr != sodi_ptr*/;
}

int spm_set_vcore_dvs_voltage(unsigned int opp)
{
	u8 f26m_req, apsrc_req;
	u32 target_sta, req;
	int timeout, r = 0;
	bool not_existed, not_support;
	unsigned long flags;

	if (opp == OPPI_PERF_ULTRA) {
		f26m_req = 1;
		apsrc_req = 1;
		target_sta = VCORE_STA_UHPM;
	} else if (opp == OPPI_PERF) {
		f26m_req = 1;
		apsrc_req = 0;
		target_sta = VCORE_STA_HPM;
	} else if (opp == OPPI_LOW_PWR) {
		f26m_req = 0;
		apsrc_req = 0;
		target_sta = VCORE_STA_LPM;
	} else {
		return -EINVAL;
	}

	spin_lock_irqsave(&__spm_lock, flags);
	not_existed = is_fw_not_existed();
	not_support = is_fw_not_support_uhpm();

	if (not_existed || (opp == OPPI_PERF_ULTRA && not_support)) {
		__go_to_vcore_dvfs(SPM_VCORE_DVFS_EN, f26m_req, apsrc_req);
	} else {
		req = spm_read(SPM_PCM_SRC_REQ) & ~(SR_PCM_F26M_REQ | SR_PCM_APSRC_REQ);
		spm_write(SPM_PCM_SRC_REQ, req | (f26m_req << 1) | apsrc_req);
	}

	if (opp < OPPI_LOW_PWR) {
		/* normal FW fetch time + 1.15->1.05->1.15->1.25V transition time */
		timeout = 2 * __spm_vcore_dvfs.pcmdesc->size + 3 * PER_OPP_DVS_US;

		r = wait_pcm_complete_dvs(get_vcore_sta() == target_sta, timeout);

		if (r >= 0) {	/* DVS pass */
			r = 0;
		} else {
			spm_err("[VcoreFS] OPP: %u (%u)(%u)\n", opp, not_existed, not_support);
			spm_dump_vcore_dvs_regs(NULL);
			BUG();
		}
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return r;
}

void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

	__go_to_vcore_dvfs(spm_flags, 0, 0);

	spm_crit("[VcoreFS] STA: 0x%x, REQ: 0x%x\n", spm_read(SPM_SLEEP_DVFS_STA), spm_read(SPM_PCM_SRC_REQ));

	spin_unlock_irqrestore(&__spm_lock, flags);
}

MODULE_DESCRIPTION("SPM-VCORE_DVFS Driver v0.2");
