#ifndef __MTK_COMMON_H__
#define __MTK_COMMON_H__

/* MTK device context driver private data */
struct mtk_config {

    /* mtk mfg mapped register */
    volatile void *mfg_register;

    struct clk *clk_mfg;
    struct clk *clk_mfg_async;

    struct clk *clk_mfg_core0;
    struct clk *clk_mfg_core1;

    struct clk *clk_mfg_main;
#ifdef ENABLE_MFG_PERF
    struct clk *clk_mfg_f52m_sel;
    bool mfg_perf_enabled;
#endif

};

#ifdef ENABLE_MFG_PERF
/* MTK HAL mtk_enable_gpu_perf_monitor implementation */
bool _mtk_enable_gpu_perf_monitor(bool enable);
#endif

#define MTKCLK_prepare_enable(clk) \
	if (config->clk) { \
		if (clk_prepare_enable(config->clk)) \
		pr_alert("MALI: clk_prepare_enable failed when enabling" #clk); }

#define MTKCLK_disable_unprepare(clk) \
	if (config->clk) { \
		clk_disable_unprepare(config->clk); }



#endif /* __MTK_COMMON_H__ */
