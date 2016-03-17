#ifndef __DDP_PWM_MUX_H__
#define __DDP_PWM_MUX_H__

int disp_pwm_set_pwmmux(unsigned int clk_req);

int disp_pwm_clksource_enable(int clk_req);

int disp_pwm_clksource_disable(int clk_req);

#endif
