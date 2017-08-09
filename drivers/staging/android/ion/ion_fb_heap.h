#ifndef __ION_FB_HEAP_H__
#define __ION_FB_HEAP_H__

typedef struct {
	struct mutex lock;
	int eModuleID;
	unsigned int security;
	unsigned int coherent;
	void *pVA;
	unsigned int MVA;
	ion_phys_addr_t priv_phys;
	ion_mm_buf_debug_info_t dbg_info;
	ion_mm_sf_buf_info_t sf_buf_info;
} ion_fb_buffer_info;

#endif
