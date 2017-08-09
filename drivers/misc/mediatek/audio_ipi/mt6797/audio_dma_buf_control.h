#ifndef AUDIO_DMA_BUF_CONTROL_H
#define AUDIO_DMA_BUF_CONTROL_H

#include <linux/types.h>

typedef struct {
	char    *phy_addr;
	char    *vir_addr;
	uint32_t size;
} audio_resv_dram_t;


void init_reserved_dram(void);
audio_resv_dram_t *get_reserved_dram(void);

char *get_resv_dram_vir_addr(char *resv_dram_phy_addr);


#endif /* end of AUDIO_DMA_BUF_CONTROL_H */

