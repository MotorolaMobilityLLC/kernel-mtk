/*
 *
 * (C) COPYRIGHT 2010-2019 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */


#include "mali_kbase.h"
#include <gpu/mali_kbase_gpu_regmap.h>
#include "mali_kbase_defs.h"

#define ENTRY_TYPE_MASK     3ULL
#define ENTRY_IS_ATE        1ULL
#define ENTRY_IS_INVAL      2ULL
#define ENTRY_IS_PTE        3ULL

#define ENTRY_ATTR_BITS (7ULL << 2)	/* bits 4:2 */
#define ENTRY_RD_BIT (1ULL << 6)
#define ENTRY_WR_BIT (1ULL << 7)
#define ENTRY_SHARE_BITS (3ULL << 8)	/* bits 9:8 */
#define ENTRY_ACCESS_BIT (1ULL << 10)
#define ENTRY_NX_BIT (1ULL << 54)

#define ENTRY_FLAGS_MASK (ENTRY_ATTR_BITS | ENTRY_RD_BIT | ENTRY_WR_BIT | \
		ENTRY_SHARE_BITS | ENTRY_ACCESS_BIT | ENTRY_NX_BIT)

#define UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR (55)
#define VALID_ENTRY_MASK ((u64)0xF << UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR)

/* Helper Function to perform assignment of page table entries, to
 * ensure the use of strd, which is required on LPAE systems.
 */
static inline void page_table_entry_set(u64 *pte, u64 phy)
{
#if KERNEL_VERSION(3, 18, 13) <= LINUX_VERSION_CODE
	WRITE_ONCE(*pte, phy);
#else
#ifdef CONFIG_64BIT
	barrier();
	*pte = phy;
	barrier();
#elif defined(CONFIG_ARM)
	barrier();
	asm volatile("ldrd r0, [%1]\n\t"
		     "strd r0, %0\n\t"
		     : "=m" (*pte)
		     : "r" (&phy)
		     : "r0", "r1");
	barrier();
#else
#error "64-bit atomic write must be implemented for your architecture"
#endif
#endif
}

static void mmu_get_as_setup(struct kbase_mmu_table *mmut,
		struct kbase_mmu_setup * const setup)
{
	/* Set up the required caching policies at the correct indices
	 * in the memattr register.
	 */
	setup->memattr =
		(AS_MEMATTR_LPAE_IMPL_DEF_CACHE_POLICY <<
		(AS_MEMATTR_INDEX_IMPL_DEF_CACHE_POLICY * 8)) |
		(AS_MEMATTR_LPAE_FORCE_TO_CACHE_ALL    <<
		(AS_MEMATTR_INDEX_FORCE_TO_CACHE_ALL * 8))    |
		(AS_MEMATTR_LPAE_WRITE_ALLOC           <<
		(AS_MEMATTR_INDEX_WRITE_ALLOC * 8))           |
		(AS_MEMATTR_LPAE_OUTER_IMPL_DEF        <<
		(AS_MEMATTR_INDEX_OUTER_IMPL_DEF * 8))        |
		(AS_MEMATTR_LPAE_OUTER_WA              <<
		(AS_MEMATTR_INDEX_OUTER_WA * 8))              |
		0; /* The other indices are unused for now */

	setup->transtab = ((u64)mmut->pgd &
		((0xFFFFFFFFULL << 32) | AS_TRANSTAB_LPAE_ADDR_SPACE_MASK)) |
		AS_TRANSTAB_LPAE_ADRMODE_TABLE |
		AS_TRANSTAB_LPAE_READ_INNER;

	setup->transcfg = 0;
}

static void mmu_update(struct kbase_device *kbdev,
		struct kbase_mmu_table *mmut,
		int as_nr)
{
	struct kbase_as *as;
	struct kbase_mmu_setup *current_setup;

	if (WARN_ON(as_nr == KBASEP_AS_NR_INVALID))
		return;

	as = &kbdev->as[as_nr];
	current_setup = &as->current_setup;

	mmu_get_as_setup(mmut, current_setup);

	/* Apply the address space setting */
	kbase_mmu_hw_configure(kbdev, as);
}

static void mmu_disable_as(struct kbase_device *kbdev, int as_nr)
{
	struct kbase_as * const as = &kbdev->as[as_nr];
	struct kbase_mmu_setup * const current_setup = &as->current_setup;

	current_setup->transtab = AS_TRANSTAB_LPAE_ADRMODE_UNMAPPED;

	/* Apply the address space setting */
	kbase_mmu_hw_configure(kbdev, as);
}

static phys_addr_t pte_to_phy_addr(u64 entry)
{
	if (!(entry & 1))
		return 0;

	entry &= ~VALID_ENTRY_MASK;
	return entry & ~0xFFF;
}

static int ate_is_valid(u64 ate, int const level)
{
	return ((ate & ENTRY_TYPE_MASK) == ENTRY_IS_ATE);
}

static int pte_is_valid(u64 pte, int const level)
{
	return ((pte & ENTRY_TYPE_MASK) == ENTRY_IS_PTE);
}

/*
 * Map KBASE_REG flags to MMU flags.
 */
static u64 get_mmu_flags(unsigned long flags)
{
	u64 mmu_flags;
	unsigned long memattr_idx;

	memattr_idx = KBASE_REG_MEMATTR_VALUE(flags);
	if (WARN(memattr_idx == AS_MEMATTR_INDEX_NON_CACHEABLE,
			"Legacy Mode MMU cannot honor GPU non-cachable memory, will use default instead\n"))
		memattr_idx = AS_MEMATTR_INDEX_DEFAULT;
	/* store mem_attr index as 4:2, noting that:
	 * - macro called above ensures 3 bits already
	 * - all AS_MEMATTR_INDEX_<...> macros only use 3 bits
	 */
	mmu_flags = memattr_idx << 2;

	/* write perm if requested */
	mmu_flags |= (flags & KBASE_REG_GPU_WR) ? ENTRY_WR_BIT : 0;
	/* read perm if requested */
	mmu_flags |= (flags & KBASE_REG_GPU_RD) ? ENTRY_RD_BIT : 0;
	/* nx if requested */
	mmu_flags |= (flags & KBASE_REG_GPU_NX) ? ENTRY_NX_BIT : 0;

	if (flags & KBASE_REG_SHARE_BOTH) {
		/* inner and outer shareable */
		mmu_flags |= SHARE_BOTH_BITS;
	} else if (flags & KBASE_REG_SHARE_IN) {
		/* inner shareable coherency */
		mmu_flags |= SHARE_INNER_BITS;
	}

	return mmu_flags;
}

static void entry_set_ate(u64 *entry,
		struct tagged_addr phy,
		unsigned long flags,
		int const level)
{
	page_table_entry_set(entry, as_phys_addr_t(phy) | get_mmu_flags(flags) |
			     ENTRY_IS_ATE);
}

static unsigned int get_num_valid_entries(u64 *pgd)
{
	register unsigned int num_of_valid_entries;
	num_of_valid_entries =
		(unsigned int)((pgd[2] & VALID_ENTRY_MASK) >>
			       (UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR - 8));
	num_of_valid_entries |=
		(unsigned int)((pgd[1] & VALID_ENTRY_MASK) >>
			       (UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR - 4));
	num_of_valid_entries |=
		(unsigned int)((pgd[0] & VALID_ENTRY_MASK) >>
			       (UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR));
	return num_of_valid_entries;
}

static void set_num_valid_entries(u64 *pgd, unsigned int num_of_valid_entries)
{
	WARN_ON_ONCE(num_of_valid_entries > KBASE_MMU_PAGE_ENTRIES);
	pgd[0] &= ~VALID_ENTRY_MASK;
	pgd[0] |= ((u64)(num_of_valid_entries & 0xF)
		   << UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR);
	pgd[1] &= ~VALID_ENTRY_MASK;
	pgd[1] |= ((u64)((num_of_valid_entries >> 4) & 0xF)
		   << UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR);
	pgd[2] &= ~VALID_ENTRY_MASK;
	pgd[2] |= ((u64)((num_of_valid_entries >> 8) & 0xF)
		   << UNUSED_BIT_POSITION_IN_PAGE_DESCRIPTOR);
}

static void entry_set_pte(u64 *pgd, u64 vpfn, phys_addr_t phy)
{
	unsigned int nr_entries = get_num_valid_entries(pgd);
	page_table_entry_set(&pgd[vpfn], (phy & PAGE_MASK) | ENTRY_ACCESS_BIT |
						 ENTRY_IS_PTE);
	set_num_valid_entries(pgd, nr_entries + 1);
}

static void entries_invalidate(u64 *entry, u32 count)
{
	u32 i;

	for (i = 0; i < count; i++)
		page_table_entry_set(entry + i, ENTRY_IS_INVAL);
}

static struct kbase_mmu_mode const lpae_mode = {
	.update = mmu_update,
	.get_as_setup = mmu_get_as_setup,
	.disable_as = mmu_disable_as,
	.pte_to_phy_addr = pte_to_phy_addr,
	.ate_is_valid = ate_is_valid,
	.pte_is_valid = pte_is_valid,
	.entry_set_ate = entry_set_ate,
	.entry_set_pte = entry_set_pte,
	.entries_invalidate = entries_invalidate,
	.get_num_valid_entries = get_num_valid_entries,
	.set_num_valid_entries = set_num_valid_entries,
	.flags = 0
};

struct kbase_mmu_mode const *kbase_mmu_mode_get_lpae(void)
{
	return &lpae_mode;
}
