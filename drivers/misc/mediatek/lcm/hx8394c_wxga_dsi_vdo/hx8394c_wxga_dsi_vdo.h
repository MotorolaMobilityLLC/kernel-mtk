#ifndef _HX8394C_WXGA_DSI_VDO_
#define _HX8394C_WXGA_DSI_VDO_

#include "ddp_hal.h"

extern unsigned int GPIO_LCD_RST;
extern unsigned int GPIO_LCD_PWR;

extern void DSI_clk_HS_mode(DISP_MODULE_ENUM module, void *cmdq, bool enter);

#endif

