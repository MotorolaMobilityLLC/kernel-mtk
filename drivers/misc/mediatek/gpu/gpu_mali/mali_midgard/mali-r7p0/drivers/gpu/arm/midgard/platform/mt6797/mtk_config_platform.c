#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/delay.h>

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <platform/mtk_platform_common.h>

#include "mali_kbase_config_platform.h"
#include "mtk_kbase_spm.h"

#ifdef ENABLE_COMMON_DVFS
#include <ged_dvfs.h>
#endif

#ifndef MTK_GPU_SPM
#include <mt_gpufreq.h>
#endif

#include <fan53555.h>

struct mtk_config *g_config;

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

static volatile void *g_ldo_base = NULL;

volatile void *g_MFG_base;
volatile void *g_DVFS_GPU_base;
volatile void *g_DFP_base;
volatile void *g_TOPCK_base;

static void mt6797_gpu_set_power(int on)
{
	if (on)
	{
		/* LDO: VSRAM_GPU */
		base_write32(g_ldo_base+0xfbc, 0x1ff);

#ifdef MTK_GPU_SPM
		/* FAN53555: VGPU */
		fan53555_config_interface(0x0, 0x1, 0x1, 0x7);
#else
		mt_gpufreq_voltage_enable_set(1);
#endif
		udelay(340);
	}
	else
	{
#ifdef MTK_GPU_SPM
		/* FAN53555: VGPU */
		fan53555_config_interface(0x0, 0x0, 0x1, 0x7);
#else
		mt_gpufreq_voltage_enable_set(0);
#endif

		/* LDO: VSRAM_GPU */
		base_write32(g_ldo_base+0xfbc, 0x0);
	}
}

#define MTKCLK_prepare_enable(clk) \
	if (config->clk) { if (clk_prepare_enable(config->clk)) \
		pr_alert("MALI: clk_prepare_enable failed when enabling " #clk ); }

#define MTKCLK_disable_unprepare(clk) \
	if (config->clk) {  clk_disable_unprepare(config->clk); }

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	struct mtk_config *config = kbdev->mtk_config;

	mt6797_gpu_set_power(1);

	MTKCLK_prepare_enable(clk_smi_common);

	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
#ifndef MTK_GPU_APM
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);
	MTKCLK_prepare_enable(clk_mfg_core2);
	MTKCLK_prepare_enable(clk_mfg_core3);
#endif
	MTKCLK_prepare_enable(clk_mfg_main);

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

#ifdef MTK_GPU_SPM
	MTKCLK_prepare_enable(clk_dvfs_gpu);
	mtk_kbase_spm_acquire();
	mtk_kbase_spm_con(SPM_RSV_BIT_EN |
		(mtk_kbase_spm_get_dvfs_en() ? SPM_RSV_BIT_DVFS_EN : 0)
		);
	mtk_kbase_spm_wait();
	mtk_kbase_spm_release();
#endif

#ifdef MTK_GPU_OCP
	mtk_kbase_dpm_setup(dfp_weights);
	MFG_write32(MFG_OCP_DCM_CON, 0x1);
#endif

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(1);
#endif

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	struct mtk_config *config = kbdev->mtk_config;

#ifdef ENABLE_COMMON_DVFS
	ged_dvfs_gpu_clock_switch_notify(0);
#endif

#ifdef MTK_GPU_OCP
	MFG_write32(MFG_OCP_DCM_CON, 0x0);
#endif

#ifdef MTK_GPU_SPM
	mtk_kbase_spm_acquire();
	mtk_kbase_spm_con(0);
	mtk_kbase_spm_wait();
	mtk_kbase_spm_release();
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

	MTKCLK_disable_unprepare(clk_smi_common);

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

    return 0;
}

#ifdef MTK_GPU_SPM

ssize_t mtk_kbase_dvfs_gpu_show(struct device *dev, struct device_attribute *attr, char *const buf)
{
	struct mtk_config *config = g_config;
	ssize_t ret = 0;
	int i, j, k;
	uint32_t dvfs_gpu_bases[] = {0x0, 0x300, 0x350, 0x3A0, 0x600, 0x800};

	/* dump LDO */
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "ldo 0xfc0 = 0x%08x\n", base_read32(g_ldo_base+0xfc0));
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "ldo 0xfc4 = 0x%08x\n", base_read32(g_ldo_base+0xfc4));
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "ldo 0xfc8 = 0x%08x\n\n", base_read32(g_ldo_base+0xfc8));

	/* dump VGPU */
	//{
	//	unsigned char x = 0;
	//	fan53555_read_interface(0x0, &x, 0xff, 0x0);
	//	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "fan53555 0x0 = 0x%x\n", x);
	//	fan53555_read_interface(0x1, &x, 0xff, 0x0);
	//	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "fan53555 0x1 = 0x%x\n", x);
	//	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	//}

	/* dump DFP registers */
	//ret += scnprintf(buf + ret, PAGE_SIZE - ret, "DFP_ctrl = %u\n", DFP_read32(DFP_CTRL));
	//ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");

	/* dump DVFS_GPU registers */
	MTKCLK_prepare_enable(clk_dvfs_gpu);

	i = mtk_kbase_spm_get_vol(SPM_SW_CUR_V);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "current voltage: %u.%06uV\n", i / 1000000, i % 1000000);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "current freqency: %uMHz\n\n", mtk_kbase_spm_get_freq(SPM_SW_CUR_F));

	i = mtk_kbase_spm_get_vol(SPM_SW_CEIL_V);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "ceiling voltage: %u.%06uV\n", i / 1000000, i % 1000000);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "cceiling freqency: %uMHz\n\n", mtk_kbase_spm_get_freq(SPM_SW_CEIL_F));

	i = mtk_kbase_spm_get_vol(SPM_SW_FLOOR_V);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "floor voltage: %u.%06uV\n", i / 1000000, i % 1000000);
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "floor freqency: %uMHz\n\n", mtk_kbase_spm_get_freq(SPM_SW_FLOOR_F));

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
	MTKCLK_disable_unprepare(clk_dvfs_gpu);

	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
		"command list\n"
		"  reset : reset to default\n"
		"  enable : enable DVFS\n"
		"  dsiable : disable DVFS\n"
		"  ceiling_fv <freq> <vol> : ex. ceiling_fv 700000 1125\n"
		"  floor_fv <freq> <vol> : ex. floor_fv 154000 800\n"
		"  fix_fv <freq> <vol> : ex. fix_fv 700000 1125\n"
		);

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
	struct mtk_config *config = g_config;
	MTKCLK_prepare_enable(clk_dvfs_gpu);

	if (strncmp(buf, "reset",  MIN(5, count)) == 0)
	{
		mtk_kbase_spm_set_dvfs_en(1);
		mtk_kbase_spm_kick(&dvfs_gpu_pcm);
	}
	else if (strncmp(buf, "enable",  MIN(6, count)) == 0)
	{
		mtk_kbase_spm_set_dvfs_en(1);
	}
	else if (strncmp(buf, "disable",  MIN(7, count)) == 0)
	{
		mtk_kbase_spm_set_dvfs_en(0);
	}
	else if (strncmp("ceiling_fv", buf, MIN(10, count)) == 0)
	{
		unsigned int v, f;
		int items = sscanf(buf, "ceiling_fv %u %u", &f, &v);
		if (items == 2 && v > 0 && f > 0)
		{
			mtk_kbase_spm_set_vol_freq_ceiling(v*1000, f/1000);
		}
	}
	else if (strncmp("floor_fv", buf, MIN(8, count)) == 0)
	{
		unsigned int v, f;
		int items = sscanf(buf, "floor_fv %u %u", &f, &v);
		if (items == 2 && v > 0 && f > 0)
		{
			mtk_kbase_spm_set_vol_freq_floor(v*1000, f/1000);
		}
	}
	else if (strncmp("fix_fv", buf, MIN(6, count)) == 0)
	{
		unsigned int v, f;
		int items = sscanf(buf, "fix_fv %u %u", &f, &v);
		if (items == 2 && v > 0 && f > 0)
		{
			mtk_kbase_spm_fix_vol_freq(v*1000, f/1000);
		}
	}

	MTKCLK_disable_unprepare(clk_dvfs_gpu);

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
	memset(config, 0, sizeof(struct mtk_config));

	/* MTK: TODO, using device_treee */
	g_ldo_base = ioremap_nocache(0x10001000, 0x1000);
	g_MFG_base = ioremap_nocache(0x13000000, 0x1000);
	g_DVFS_GPU_base = ioremap_nocache(0x11016000, 0x1000);
	g_DFP_base = ioremap_nocache(0x13020000, 0x1000);
	g_TOPCK_base = ioremap_nocache(0x10000000, 0x1000);

	config->clk_smi_common = devm_clk_get(&pdev->dev, "mfg-smi-common");

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

	dev_err(kbdev->dev, "xxxx smi_common:%p\n", config->clk_smi_common);

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

	config->max_vol = 1125000;
	config->max_freq = 700;
	config->min_vol = 800000;
	config->min_freq = 154;
	config->slope = 595;

	g_config = kbdev->mtk_config = config;

#ifdef MTK_GPU_SPM
	if (device_create_file(kbdev->dev, &dev_attr_dvfs_gpu_dump))
	{
		dev_err(kbdev->dev, "xxxx dvfs_gpu_dump create fail\n");
	}
#endif

#ifdef MTK_GPU_SPM
	MTKCLK_prepare_enable(clk_gpupm);
	MTKCLK_prepare_enable(clk_dvfs_gpu);

	mtk_kbase_spm_kick(&dvfs_gpu_pcm);

	MTKCLK_disable_unprepare(clk_dvfs_gpu);
#endif

	base_write32(g_ldo_base+0xfc0, 0x0f0f0f0f);
	base_write32(g_ldo_base+0xfc4, 0x0f0f0f0f);
	base_write32(g_ldo_base+0xfc8, 0x0f);

	fan53555_config_interface(2, 5, 7, 4);

#ifdef MTK_GPU_DPM
	DFP_write32(CLK_MISC_CFG_0, DFP_read32(CLK_MISC_CFG_0) & 0xfffffffe);
#endif

	return 0;
}

