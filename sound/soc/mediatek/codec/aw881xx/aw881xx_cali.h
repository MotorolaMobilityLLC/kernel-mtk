#ifndef __AW881XX_CALI_H__
#define __AW881XX_CALI_H__

#define AW_CALI_STORE_EXAMPLE

#define AW_ERRO_CALI_VALUE (0)

struct aw881xx_cali_attr {
	uint32_t cali_re;
	uint32_t re;
	uint32_t f0;
};

void aw881xx_set_cali_re_to_dsp(struct aw881xx_cali_attr *cali_attr);
void aw881xx_get_cali_re(struct aw881xx_cali_attr *cali_attr);
void aw881xx_cali_init(struct aw881xx_cali_attr *cali_attr);

#endif
