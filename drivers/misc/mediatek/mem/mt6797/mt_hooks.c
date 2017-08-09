#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/printk.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/atomic.h>

static atomic_t reg_0x1001Axxx_cnt = ATOMIC_INIT(1);

void ioremap_debug_hook_func(phys_addr_t phys_addr, size_t size,
		unsigned int mtype)
{
	unsigned long offset = phys_addr & ~PAGE_MASK;
	phys_addr_t target = 0x1001A000;

	target &= PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/* only allow 1st ioremap to address 0x1001AXXX */
	if ((phys_addr + size <= target) ||
			(phys_addr >= (target + PAGE_SIZE)))
		return;

	if (atomic_dec_and_test(&reg_0x1001Axxx_cnt))
		return;

	/* trap other ioremap to 0x1001AXXX */
	pr_err("ioremap to 0x1001AXXX more than once (%pa, %zx)\n",
			&phys_addr, size);
	BUG();
}
