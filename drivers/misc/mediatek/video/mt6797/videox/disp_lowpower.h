#ifndef _DISP_LOWPOWER_H_
#define _DISP_LOWPOWER_H_

unsigned int dsi_phy_get_clk(DISP_MODULE_ENUM module);
void primary_display_idlemgr_enter_idle_nolock(void);


golden_setting_context *get_golden_setting_pgc(void);
int primary_display_lowpower_init(void);
void primary_display_sodi_rule_init(void);
/*
return 0: display is not idle trigger now
return 1: display is idle
*/
int primary_display_is_idle(void);
void primary_display_idlemgr_kick(const char *source, int need_lock);
void enable_sodi_by_cmdq(cmdqRecHandle handler);
void disable_sodi_by_cmdq(cmdqRecHandle handler);
void enter_share_sram(CMDQ_EVENT_ENUM resourceEvent);
void leave_share_sram(CMDQ_EVENT_ENUM resourceEvent);
void set_hrtnum(unsigned int new_hrtnum);
void set_enterulps(unsigned flag);
void set_is_dc(unsigned int is_dc);
/**************************************** for met******************************************* */
/*
return 0: not enter ultra lowpower state which means mipi pll enable
return 1: enter ultra lowpower state whicn means mipi pll disable
*/
unsigned int is_enterulps(void);

/*
read dsi regs to calculate clk
*/
unsigned int get_clk(void);

#endif
