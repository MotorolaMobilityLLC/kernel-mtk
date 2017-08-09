
#ifndef __MTK_OVL_H__
#define __MTK_OVL_H__
#include "primary_display.h"
void ovl2mem_setlayernum(int layer_num);
int ovl2mem_get_info(void *info);
int get_ovl2mem_ticket(void);
int ovl2mem_init(unsigned int session);
int ovl2mem_input_config(disp_session_input_config *input);
int ovl2mem_output_config(disp_mem_output_config *out);
int ovl2mem_trigger(int blocking, void *callback, unsigned int userdata);
void ovl2mem_wait_done(void);
int ovl2mem_deinit(void);

#endif
