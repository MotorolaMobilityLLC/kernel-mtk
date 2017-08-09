#ifndef __MT_C2K_SDIO_H__
#define __MT_C2K_SDIO_H__

#include <linux/pm.h>

typedef void (*sdio_irq_handler_t)(void *);
typedef void (*pm_callback_t)(pm_message_t state, void *data);

static void c2k_sdio_request_eirq(sdio_irq_handler_t irq_handler, void *data)
{
	pr_err("to be implemented\n");
}

static void c2k_sdio_enable_eirq(void)
{
	pr_err("to be implemented\n");
}

static void c2k_sdio_disable_eirq(void)
{
	pr_err("to be implemented\n");
}

static void c2k_sdio_register_pm(pm_callback_t pm_cb, void *data)
{
	pr_err("to be implemented\n");
}

#endif
