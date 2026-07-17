// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 */

#include <hyp/debug-sr.h>

#include <linux/compiler.h>
#include <linux/kvm_host.h>

#include <asm/debug-monitors.h>
#include <asm/kvm_asm.h>
#include <asm/kvm_hyp.h>
#include <asm/kvm_mmu.h>

static void __debug_save_spe(u64 *pmscr_el1, u64 *pmblimitr_el1)
{
	/*
	 * At this point, we know that this CPU implements
	 * SPE and is available to the host.
	 * Check if the host is actually using it ?
	 */
	*pmblimitr_el1 = read_sysreg_s(SYS_PMBLIMITR_EL1);
	if (!(*pmblimitr_el1 & BIT(PMBLIMITR_EL1_E_SHIFT)))
		return;

	/* Yes; save the control register and disable data generation */
	*pmscr_el1 = read_sysreg_s(SYS_PMSCR_EL1);
	write_sysreg_s(0, SYS_PMSCR_EL1);
	isb();

	/* Now drain all buffered data to memory */
	psb_csync();
	dsb(nsh);

	/* And disable the profiling buffer */
	write_sysreg_s(0, SYS_PMBLIMITR_EL1);
	isb();
}

static void __debug_restore_spe(u64 pmscr_el1, u64 pmblimitr_el1)
{
	if (!(pmblimitr_el1 & BIT(PMBLIMITR_EL1_E_SHIFT)))
		return;

	/* The host page table is installed, but not yet synchronised */
	isb();

	/* Re-enable the profiling buffer. */
	write_sysreg_s(pmblimitr_el1, SYS_PMBLIMITR_EL1);
	isb();

	/* Re-enable data generation */
	write_sysreg_s(pmscr_el1, SYS_PMSCR_EL1);
}

static void __debug_save_trace(u64 *trfcr_el1, u64 *trblimitr_el1)
{
	/*
	 * Prohibit trace generation while we are in guest.
	 * Since access to TRFCR_EL1 is trapped, the guest can't
	 * modify the filtering set by the host.
	 */
	*trfcr_el1 = read_sysreg_s(SYS_TRFCR_EL1);
	write_sysreg_s(0, SYS_TRFCR_EL1);

	/* Check if the TRBE is enabled */
	*trblimitr_el1 = read_sysreg_s(SYS_TRBLIMITR_EL1);
	if (*trblimitr_el1 & TRBLIMITR_EL1_E) {
		/*
		 * The host has enabled the Trace Buffer Unit so we have
		 * to beat the CPU with a stick until it stops accessing
		 * memory.
		 */

		/* First, ensure that our prior write to TRFCR has stuck. */
		isb();

		/* Now synchronise with the trace and drain the buffer. */
		tsb_csync();
		dsb(nsh);

		/*
		 * With no more trace being generated, we can disable the
		 * Trace Buffer Unit.
		 */
		write_sysreg_s(0, SYS_TRBLIMITR_EL1);
		if (cpus_have_final_cap(ARM64_WORKAROUND_2064142)) {
			/*
			 * Some CPUs are so good, we have to drain 'em
			 * twice.
			 */
			tsb_csync();
			dsb(nsh);
		}

		/*
		 * Ensure that the Trace Buffer Unit is disabled before
		 * we start mucking with the stage-2 and trap
		 * configuration.
		 */
		isb();
	}
}

static void __debug_restore_trace(u64 trfcr_el1, u64 trblimitr_el1)
{
	if (trblimitr_el1 & TRBLIMITR_EL1_E) {
		/* Re-enable the Trace Buffer Unit for the host. */
		write_sysreg_s(trblimitr_el1, SYS_TRBLIMITR_EL1);
		isb();
		if (cpus_have_final_cap(ARM64_WORKAROUND_2038923)) {
			/*
			 * Make sure the unit is re-enabled before we
			 * poke TRFCR.
			 */
			isb();
		}
	}

	/* Restore trace filter controls */
	write_sysreg_s(trfcr_el1, SYS_TRFCR_EL1);
}

void __debug_save_host_buffers_nvhe(struct kvm_vcpu *vcpu)
{
	struct kvm_host_data *host_data = this_cpu_ptr(&kvm_host_data);
	bool save_spe, save_trbe;

	if (is_protected_kvm_enabled()) {
		u64 dfr0 = read_sysreg(id_aa64dfr0_el1);

		save_spe = FIELD_GET(ID_AA64DFR0_EL1_PMSVer, dfr0) &&
			   !(read_sysreg_s(SYS_PMBIDR_EL1) & PMBIDR_EL1_P);
		save_trbe = FIELD_GET(ID_AA64DFR0_EL1_TraceBuffer, dfr0) &&
			    !(read_sysreg_s(SYS_TRBIDR_EL1) & TRBIDR_EL1_P);
	} else {
		save_spe = vcpu_get_flag(vcpu, DEBUG_STATE_SAVE_SPE);
		save_trbe = vcpu_get_flag(vcpu, DEBUG_STATE_SAVE_TRBE);
	}

	/* Disable and flush SPE data generation */
	if (save_spe)
		__debug_save_spe(&vcpu->arch.host_debug_state.pmscr_el1,
		                 &host_data->pmblimitr_el1);
	/* Disable and flush Self-Hosted Trace generation */
	if (save_trbe)
		__debug_save_trace(&vcpu->arch.host_debug_state.trfcr_el1,
		                   &host_data->trblimitr_el1);
}
void __debug_switch_to_guest(struct kvm_vcpu *vcpu)
{
	__debug_switch_to_guest_common(vcpu);
}

void __debug_restore_host_buffers_nvhe(struct kvm_vcpu *vcpu)
{
	struct kvm_host_data *host_data = this_cpu_ptr(&kvm_host_data);
	bool restore_spe, restore_trbe;

	if (is_protected_kvm_enabled()) {
		u64 dfr0 = read_sysreg(id_aa64dfr0_el1);

		restore_spe = FIELD_GET(ID_AA64DFR0_EL1_PMSVer, dfr0) &&
			      !(read_sysreg_s(SYS_PMBIDR_EL1) & PMBIDR_EL1_P);
		restore_trbe = FIELD_GET(ID_AA64DFR0_EL1_TraceBuffer, dfr0) &&
			       !(read_sysreg_s(SYS_TRBIDR_EL1) & TRBIDR_EL1_P);
	} else {
		restore_spe = vcpu_get_flag(vcpu, DEBUG_STATE_SAVE_SPE);
		restore_trbe = vcpu_get_flag(vcpu, DEBUG_STATE_SAVE_TRBE);
	}

	if (restore_spe)
		__debug_restore_spe(vcpu->arch.host_debug_state.pmscr_el1,
		                    host_data->pmblimitr_el1);
	if (restore_trbe)
		__debug_restore_trace(vcpu->arch.host_debug_state.trfcr_el1,
		                      host_data->trblimitr_el1);
}

void __debug_switch_to_host(struct kvm_vcpu *vcpu)
{
	__debug_switch_to_host_common(vcpu);
}

u64 __kvm_get_mdcr_el2(void)
{
	return read_sysreg(mdcr_el2);
}
