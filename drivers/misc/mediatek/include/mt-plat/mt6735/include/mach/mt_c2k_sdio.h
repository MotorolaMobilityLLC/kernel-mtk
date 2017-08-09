#ifndef __MT_C2K_SDIO_H__
#define __MT_C2K_SDIO_H__

#include <linux/pm.h>

#include <linux/mmc/sdio_func.h>

typedef void (*pm_callback_t)(pm_message_t state, void *data);

extern void c2k_sdio_request_eirq(sdio_irq_handler_t irq_handler, void *data);
extern void c2k_sdio_enable_eirq(void);
extern void c2k_sdio_disable_eirq(void);
extern void c2k_sdio_register_pm(pm_callback_t pm_cb, void *data);
/*
extern void c2k_sdio_install_eirq(void);
extern void c2k_sdio_uninstall_eirq(void);
extern void via_sdio_on (int sdio_port_num);
extern void via_sdio_off (int sdio_port_num);
*/

#endif
