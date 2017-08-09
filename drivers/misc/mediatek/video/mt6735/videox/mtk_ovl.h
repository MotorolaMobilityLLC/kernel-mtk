
#ifndef __MTK_OVL_H__
#define __MTK_OVL_H__
#include "primary_display.h"
typedef struct {
	unsigned int layer;
	unsigned int layer_en;
	unsigned int buffer_source;
	unsigned int fmt;
	unsigned long addr;
	unsigned long addr_sub_u;
	unsigned long addr_sub_v;
	unsigned long vaddr;
	unsigned int src_x;
	unsigned int src_y;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_pitch;
	unsigned int dst_x;
	unsigned int dst_y;
	unsigned int dst_w;
	unsigned int dst_h;	/* clip region */
	unsigned int keyEn;
	unsigned int key;
	unsigned int aen;
	unsigned char alpha;

	unsigned int sur_aen;
	unsigned int src_alpha;
	unsigned int dst_alpha;

	unsigned int isTdshp;
	unsigned int isDirty;

	unsigned int buff_idx;
	unsigned int identity;
	unsigned int connected_type;
	unsigned int security;
	unsigned int dirty;
} ovl2mem_in_config;

typedef struct {
	unsigned int fmt;
	unsigned long addr;
	unsigned long addr_sub_u;
	unsigned long addr_sub_v;
	unsigned long vaddr;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	unsigned int pitch;
	unsigned int pitchUV;

	unsigned int buff_idx;
	unsigned int security;
	unsigned int dirty;
	int mode;
} ovl2mem_io_config;

void ovl2mem_setlayernum(int layer_num);
int ovl2mem_get_info(void *info);
int get_ovl2mem_ticket(void);
int ovl2mem_init(unsigned int session);
int ovl2mem_input_config(ovl2mem_in_config *input);
int ovl2mem_output_config(disp_mem_output_config *out);
int ovl2mem_trigger(int blocking, void *callback, unsigned int userdata);
void ovl2mem_wait_done(void);
int ovl2mem_deinit(void);

#endif
