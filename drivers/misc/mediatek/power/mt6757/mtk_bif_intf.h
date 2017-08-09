#ifndef __MTK_BIF_INTF_H
#define __MTK_BIF_INTF_H

extern int mtk_bif_init(void);
extern int mtk_bif_get_vbat(unsigned int *vbat);
extern int mtk_bif_get_tbat(int *tbat);

#endif /* __MTK_BIF_INTF_H */
