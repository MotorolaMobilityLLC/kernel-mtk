#ifndef __MTK_COMMON_H__
#define __MTK_COMMON_H__

/* MTK device context driver private data */
struct mtk_config {
    
    /* mtk mfg mapped register */
    void __iomem *mfg_register;
    
    /* main clock for gpu */
    struct clk *clk_mfg;
    /* smi clock */
    struct clk *clk_smi_common;
    /* mfg MTCMOS */
    struct clk *clk_mfg_scp;
    /* display MTCMOS */
    struct clk *clk_display_scp;
    
};


#endif /* __MTK_COMMON_H__ */ 
