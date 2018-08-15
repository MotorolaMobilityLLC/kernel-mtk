/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <mt-plat/aee.h>
#include <linux/interrupt.h>
#include <mt-plat/sync_write.h>
#include "adsp_ipi.h"
#include "adsp_helper.h"
#include "adsp_excep.h"
#include "adsp_dvfs.h"


/*
 * handler for wdt irq for adsp
 * dump adsp register
 */
static int reboot;

irqreturn_t  adsp_A_wdt_handler(int irq, void *dev_id)
{
#ifdef CFG_RECOVERY_SUPPORT
	int retry;
#endif
	unsigned int reg = readl(ADSP_A_WDT_REG);

	reg &= 0x7FFFFFFF; /* set WDT_EN(bit31) to 0 */
	writel(reg, ADSP_A_WDT_REG);

	pr_debug("[ADSP] WDT exception\n");
	adsp_aed_reset(EXCEP_RUNTIME, ADSP_A_ID);
	reboot = 1;
	/* clear spm wakeup src */
	writel(0x0, ADSP_TO_SPM_REG);
#ifdef CFG_RECOVERY_SUPPORT

	/* clr after ADSP side INT trigger, or ADSP may lost INT
	 * max wait 5000*40u = 200ms
	 */
	for (retry = ADSP_AWAKE_TIMEOUT; retry > 0; retry--) {
#ifdef Liang_Check
		spin_lock_irqsave(&adsp_awake_spinlock, spin_flags);
		tmp = readl(ADSP_A_REBOOT);
		spin_unlock_irqrestore(&adsp_awake_spinlock, spin_flags);
		if (tmp == ADSP_A_READY_TO_REBOOT)
			break;
#endif
		udelay(40);
	}
	if (retry == 0)
		pr_debug("[ADSP] ADSP_A wakeup timeout\n");
	udelay(10);
#endif

	return IRQ_HANDLED;
}

/*
 * dispatch adsp irq
 * reset adsp and generate exception if needed
 * @param irq:      irq id
 * @param dev_id:   should be NULL
 */
irqreturn_t adsp_A_irq_handler(int irq, void *dev_id)
{
	if (!reboot)
		adsp_A_ipi_handler();
	/* write 1 clear */
	writel(ADSP_IRQ_ADSP2HOST, ADSP_A_TO_HOST_REG);

	return IRQ_HANDLED;
}

/*
 * adsp irq initialize
 */
void adsp_A_irq_init(void)
{
	writel(ADSP_IRQ_ADSP2HOST, ADSP_A_TO_HOST_REG); /* clear adsp irq */
}
