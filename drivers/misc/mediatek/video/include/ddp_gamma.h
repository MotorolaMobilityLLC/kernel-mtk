#ifndef __DDP_GAMMA_H__
#define __DDP_GAMMA_H__

#include <asm/uaccess.h>


typedef enum {
	DISP_GAMMA0 = 0,
	DISP_GAMMA_TOTAL
} disp_gamma_id_t;


typedef unsigned int gamma_entry;
#define GAMMA_ENTRY(r10, g10, b10) (((r10) << 20) | ((g10) << 10) | (b10))

#define DISP_GAMMA_LUT_SIZE 512

typedef struct {
	disp_gamma_id_t hw_id;
	gamma_entry lut[DISP_GAMMA_LUT_SIZE];
} DISP_GAMMA_LUT_T;


typedef enum {
	DISP_CCORR0 = 0,
	DISP_CCORR_TOTAL
} disp_ccorr_id_t;

typedef struct {
	disp_ccorr_id_t hw_id;
	unsigned int coef[3][3];
} DISP_CCORR_COEF_T;

extern int corr_dbg_en;

void ccorr_test(const char *cmd, char *debug_output);
int ccorr_interface_for_color(unsigned int ccorr_idx,
	unsigned int ccorr_coef[3][3], void *handle);

#endif

