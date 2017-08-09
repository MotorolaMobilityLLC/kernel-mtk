#ifndef __DDP_PWM_MUX_H__
#define __DDP_PWM_MUX_H__
#include <ddp_clkmgr.h>

eDDP_CLK_ID disp_pwm_get_clkid(unsigned int clk_req);

#if defined(CONFIG_ARCH_MT6755)
int disp_pwm_osc_on(void);
#endif

#endif
