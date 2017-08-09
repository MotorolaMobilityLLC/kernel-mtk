#include <linux/device.h>
#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"
#include "mtk_kbase_spm.h"

#include <platform/mtk_platform_common.h>

#include <mt_gpufreq.h>
#include <fan53555.h>

//#define MTK_GPU_DPM
//#define MTK_GPU_SPM
//#define MTK_GPU_APM
//#define MTK_GPU_OCP

#ifdef MTK_GPU_DPM
static int dfp_weights[] = {
    /*   0 */   509,    0,      0,     0,     0,     0,     0,     0, 43186, 11638,
    /*  10 */     0,  6217,  6904,   516, 24459,     0,     0,  4319,  7719, 39440,
    /*  20 */     0, 19952, 40738, 35928,     0,     0,     0,     0,     0,     0,
    /*  30 */     0,  3874, 21470, 11670,     0,     0,  6217,  6904,   516, 24459,
    /*  40 */     0,     0,  4319,  7719, 39440,     0, 19952, 40738, 35928,     0,
    /*  50 */     0,     0,     0,     0,     0,     0,  3874, 21470, 11670,     0,
    /*  60 */     0,  6217,  6904,   516, 24459,     0,     0,  4319,  7719, 39440,
    /*  70 */     0, 19952, 40738, 35928,     0,     0,     0,     0,     0,     0,
    /*  80 */     0,  3874, 21470, 11670,     0,     0,  6217,  6904,   516, 24459,
    /*  90 */     0,     0,  4319,  7719, 39440,     0, 19952, 47038, 35928,     0,
    /* 100 */     0,     0,     0,     0,     0,     0,  3874, 21470, 11670,     0
};
#endif

#ifdef MTK_GPU_SPM
static const u32 dvfs_gpu_binary[] = {
        0xe8230000, 0x11016300, 0x00aae600, 0x009bf1e0, 0x006f1580, 0x004f0a60,
        0x00334500, 0x00222e00, 0xe8230000, 0x11016328, 0x00000006, 0x00000005,
        0x00000004, 0x00000003, 0x00000002, 0x00000001, 0xe82a0000, 0x11016350,
        0x00000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0xe8208000, 0x13020004, 0x00000003, 0xe8208000,
        0x13020008, 0x00000001, 0xe8208000, 0x1302000c, 0x00000001, 0x1890001f,
        0x13020230, 0x08d00002, 0x11016354, 0x1900001f, 0x1101635c, 0x09100004,
        0x11016358, 0x19700004, 0x17c07c1f, 0x01601403, 0x18c0001f, 0x11016354,
        0xe0c00005, 0xe1000002, 0x1890001f, 0x11016358, 0x08800002, 0x00000004,
        0x49200002, 0x00000044, 0xd8000904, 0x17c07c1f, 0x1880001f, 0x00000000,
        0x18c0001f, 0x11016358, 0xe0c00002, 0x1880001f, 0x00000000, 0x18d0001f,
        0x11016354, 0x1980001f, 0x11016300, 0x1151081f, 0x01401406, 0x19300005,
        0x17c07c1f, 0x31201003, 0xd8000bc4, 0x17c07c1f, 0x08800002, 0x00000001,
        0x49300002, 0x11016350, 0xd8000a24, 0x17c07c1f, 0x1091081f, 0x09000002,
        0x11016328, 0x18f00004, 0x17c07c1f, 0x79200003, 0x00000000, 0xc8c00ec4,
        0x17c07c1f, 0x1890001f, 0x11016350, 0x18d0001f, 0x11016354, 0x1910001f,
        0x11016358, 0x1950001f, 0x13020230, 0x1990001f, 0x11016328, 0x1b80001f,
        0x20006590, 0xd0000580, 0x17c07c1f, 0xf0000000, 0xf0000000, 0x17c07c1f
};
static struct pcm_desc dvfs_gpu_pcm = {
        .version        = "SPM_PCMASM_JADE_20150421",
        .base           = dvfs_gpu_binary,
        .size           = 120,
        .sess           = 1,
        .replace        = 1,
};
#endif

struct mtk_config {
	struct clk *clk_mfg_async;
	struct clk *clk_mfg;
#ifndef MTK_MALI_APM
	struct clk *clk_mfg_core0;
	struct clk *clk_mfg_core1;
	struct clk *clk_mfg_core2;
	struct clk *clk_mfg_core3;
#endif
	struct clk *clk_mfg_main;
#ifdef MTK_GPU_SPM
	struct clk *clk_dvfs_gpu;
	struct clk *clk_gpupm;
	struct clk *clk_ap_dma;
#endif
};

struct mtk_config *g_config;

static volatile void *g_ldo_base = NULL;

volatile void *g_MFG_base;
volatile void *g_DVFS_GPU_base;
volatile void *g_DFP_base;
volatile void *g_TOPCK_base;

static void mt6797_gpu_set_power(int on)
{
}


#define MTKCLK_prepare_enable(clk) \
    if (config->clk && clk_prepare_enable(config->clk)) \
        pr_alert("MALI: clk_prepare_enable failed when enabling " #clk );

#define MTKCLK_disable_unprepare(clk) \
    if (config->clk) clk_disable_unprepare(config->clk);

static void mtk_init_registers_once(void)
{
	static int init = 0;

	if (init == 1) return;
	init = 1;

#ifdef MTK_GPU_APM
	/* enable GPU hot-plug */
	MFG_write32(0x400, MFG_read32(0x400) | 0x1);
	MFG_write32(0x418, MFG_read32(0x418) | 0x1);
	MFG_write32(0x430, MFG_read32(0x430) | 0x1);
	MFG_write32(0x448, MFG_read32(0x448) | 0x1);
#endif

	/* enable PMU */
	MFG_write32(0x3e0, 0xffffffff);
	MFG_write32(0x3e4, 0xffffffff);
	MFG_write32(0x3e8, 0xffffffff);
	MFG_write32(0x3ec, 0xffffffff);
	MFG_write32(0x3f0, 0xffffffff);

#ifdef MTK_GPU_DPM
	DFP_write32(CLK_MISC_CFG_0, DFP_read32(CLK_MISC_CFG_0) & 0xfffffffe);
#endif

#ifdef MTK_GPU_SPM
	mtk_kbase_spm_kick(&dvfs_gpu_pcm);
#endif
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	struct mtk_config *config = kbdev->mtk_config;

	mt6797_gpu_set_power(1);

	{ 	/* init VGPU power once */
		static int init = 0;

		if (init == 0)
		{
			unsigned char x0 = 0, x1;

			/* LDO: VSRAM_GPU */
			base_write32(g_ldo_base+0xfc0, 0x0f0f0f0f);
			base_write32(g_ldo_base+0xfc4, 0x0f0f0f0f);
			base_write32(g_ldo_base+0xfbc, 0xff);

			/* FAN53555: VGPU */
			fan53555_read_interface(0x0, &x0, 0xff, 0x0);
			fan53555_read_interface(0x1, &x1, 0xff, 0x0);
			dev_err(kbdev->dev, "xxxx vgpu:0x00:0x%02x, 0x01:0x%02x\n", x0, x1);

			fan53555_config_interface(0x0, 0x1, 0x1, 0x7);
			fan53555_config_interface(0x1, 0x1, 0x1, 0x7);

			fan53555_read_interface(0x0, &x0, 0xff, 0x0);
			fan53555_read_interface(0x1, &x1, 0xff, 0x0);
			dev_err(kbdev->dev, "xxxx vgpu:0x00:0x%02x, 0x01:0x%02x\n", x0, x1);

			init = 1;
		}
	}

	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
#ifndef MTK_GPU_APM
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);
	MTKCLK_prepare_enable(clk_mfg_core2);
	MTKCLK_prepare_enable(clk_mfg_core3);
#endif
	MTKCLK_prepare_enable(clk_mfg_main);

#ifdef MTK_GPU_SPM
	MTKCLK_prepare_enable(clk_dvfs_gpu);
	MTKCLK_prepare_enable(clk_gpupm);
	MTKCLK_prepare_enable(clk_ap_dma);
#endif

	mtk_init_registers_once();

#ifdef MTK_GPU_OCP
	mtk_kbase_dpm_setup(dfp_weights);
	MFG_write32(MFG_OCP_DCM_CON, 0x1);
#endif

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	struct mtk_config *config = kbdev->mtk_config;

#ifdef MTK_GPU_OCP
	MFG_write32(MFG_OCP_DCM_CON, 0x0);
#endif

#ifdef MTK_GPU_SPM
	MTKCLK_disable_unprepare(clk_ap_dma);
	MTKCLK_disable_unprepare(clk_gpupm);
	MTKCLK_disable_unprepare(clk_dvfs_gpu);
#endif

	MTKCLK_disable_unprepare(clk_mfg_main);
#ifndef MTK_GPU_APM
	MTKCLK_disable_unprepare(clk_mfg_core3);
	MTKCLK_disable_unprepare(clk_mfg_core2);
	MTKCLK_disable_unprepare(clk_mfg_core1);
	MTKCLK_disable_unprepare(clk_mfg_core0);
#endif
	MTKCLK_disable_unprepare(clk_mfg);
	MTKCLK_disable_unprepare(clk_mfg_async);

	mt6797_gpu_set_power(0);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}


int kbase_platform_early_init(void)
{
    /* Make sure the power control is ready */
    if (!is_fan53555_exist() || !is_fan53555_sw_ready())
    {
        return -EPROBE_DEFER;
    }

    /* MTK: TODO, using device_treee */
    g_ldo_base = ioremap_nocache(0x10001000, 0x1000);
    g_MFG_base = ioremap_nocache(0x13000000, 0x1000);
    g_DVFS_GPU_base = ioremap_nocache(0x11016000, 0x1000);
    g_DFP_base = ioremap_nocache(0x13020000, 0x1000);
    g_TOPCK_base = ioremap_nocache(0x10000000, 0x1000);

    return 0;
}

#ifdef MTK_GPU_SPM
ssize_t mtk_kbase_dvfs_gpu_show(struct device *dev, struct device_attribute *attr, char *const buf)
{
	int i, j, k;
	ssize_t ret = 0;

	struct mtk_config *config = g_config;
	uint32_t dvfs_gpu_bases[] = {0x0, 0x300, 0x350, 0x3A0, 0x800};

	{
		unsigned char x = 0;
		fan53555_read_interface(0x0, &x, 0xff, 0x0);
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "fan53555 0x0 = 0x%x\n", x);
		fan53555_read_interface(0x1, &x, 0xff, 0x0);
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "fan53555 0x1 = 0x%x\n", x);
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}

	/* dump DFP registers */
	//ret += scnprintf(buf + ret, PAGE_SIZE - ret, "DFP_ctrl = %u\n", DFP_read32(DFP_CTRL));
	//ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");

	/* dump DVFS_GPU registers */
	MTKCLK_prepare_enable(clk_dvfs_gpu);
	MTKCLK_prepare_enable(clk_gpupm);
	MTKCLK_prepare_enable(clk_ap_dma);
	for (k = 0; k < ARRAY_SIZE(dvfs_gpu_bases); ++k)
	{
		for (i = 0; i < 5; ++i)
		{
			uint32_t offset = dvfs_gpu_bases[k] + i * 4 * 4;

			ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[0x%03x]:", offset);
			for (j = 0; j < 4; ++j)
			{
				ret += scnprintf(buf + ret, PAGE_SIZE - ret, " 0x%08x", DVFS_GPU_read32(offset+j*4));
			}
			ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
		}
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}
	MTKCLK_disable_unprepare(clk_ap_dma);
	MTKCLK_disable_unprepare(clk_gpupm);
	MTKCLK_disable_unprepare(clk_dvfs_gpu);

	if (ret < PAGE_SIZE - 1)
	{
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}
	else
	{
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}
	return ret;
}

ssize_t mtk_kbase_dvfs_gpu_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
#if 0
	if (strncmp(buf, "reset",  MIN(5, count)) == 0)
	{
	}
	else if (!strncmp("thd=", buf, MIN(4, count)))
	{
		int thd;
		int items = sscanf(buf, "thd=%u", &thd);

		if (items == 1 && thd > 0)
		{
		}
	}
#endif

    return count;
}
static DEVICE_ATTR(dvfs_gpu_dump, S_IRUGO | S_IWUSR, mtk_kbase_dvfs_gpu_show, mtk_kbase_dvfs_gpu_set);
#endif

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{
	struct mtk_config* config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);

	if (config == NULL)
	{
		return -1;
	}

	config->clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
	config->clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
#ifndef MTK_GPU_APM
	config->clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
	config->clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
	config->clk_mfg_core2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core2");
	config->clk_mfg_core3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core3");
#endif
	config->clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");
#ifdef MTK_GPU_SPM
	config->clk_dvfs_gpu = devm_clk_get(&pdev->dev, "infra-dvfs-gpu");
	config->clk_gpupm = devm_clk_get(&pdev->dev, "infra-gpupm");
	config->clk_ap_dma = devm_clk_get(&pdev->dev, "infra-ap-dma");
#endif

	dev_err(kbdev->dev, "xxxx mfg_async:%p\n", config->clk_mfg_async);
	dev_err(kbdev->dev, "xxxx mfg:%p\n", config->clk_mfg);
#ifndef MTK_GPU_APM
	dev_err(kbdev->dev, "xxxx mfg_core0:%p\n", config->clk_mfg_core0);
	dev_err(kbdev->dev, "xxxx mfg_core1:%p\n", config->clk_mfg_core1);
	dev_err(kbdev->dev, "xxxx mfg_core2:%p\n", config->clk_mfg_core2);
	dev_err(kbdev->dev, "xxxx mfg_core3:%p\n", config->clk_mfg_core3);
#endif
	dev_err(kbdev->dev, "xxxx mfg_main:%p\n", config->clk_mfg_main);
#ifdef MTK_GPU_SPM
	dev_err(kbdev->dev, "xxxx dvfs_gpu:%p\n", config->clk_dvfs_gpu);
	dev_err(kbdev->dev, "xxxx clk_gpupm:%p\n", config->clk_gpupm);
	dev_err(kbdev->dev, "xxxx clk_ap_dmau:%p\n", config->clk_ap_dma);
#endif

	g_config = kbdev->mtk_config = config;

#ifdef MTK_GPU_SPM
	if (device_create_file(kbdev->dev, &dev_attr_dvfs_gpu_dump))
	{
		dev_err(kbdev->dev, "xxxx dvfs_gpu_dump create fail\n");
	}
#endif

	return 0;
}

