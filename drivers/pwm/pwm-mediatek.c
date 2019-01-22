/*******************************************************************************
 *  PWM Drvier
 *
 * Mediatek Pulse Width Modulator driver
 *
 * Copyright (C) 2015 John Crispin <blogic at openwrt.org>
 * Copyright (C) 2017 Zhi Mao <zhi.mao@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public Licence,
 * version 2, as publish by the Free Software Foundation.
 *
 * This program is distributed and in hope it will be useful, but WITHOUT
 * ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#ifdef PWM_ERR
#undef PWM_ERR
#endif
#define	PWM_ERR(dev, fmt, args...)	dev_info(dev, fmt, ##args)

#define PWM_EN_REG          0x0000
#define PWMCON              0x00
#define PWMGDUR             0x0c
#define PWMWAVENUM          0x28
#define PWMDWIDTH           0x2c
#define PWMTHRES            0x30
#define PWM_SEND_WAVENUM    0x34

#define PWM_CLK_DIV_MAX     7
#define PWM_NUM_MAX         8

#define PWM_CLK_NAME_MAIN   "main"
#define PWM_CLK_NAME_TOP    "top"

static const char * const mtk_pwm_clk_name[PWM_NUM_MAX] = {
	"pwm1", "pwm2", "pwm3", "pwm4", "pwm5", "pwm6", "pwm7", "pwm8"
};

struct mtk_com_pwm_data {
	const unsigned long *pwm_register;
	unsigned int pwm_nums;
};

struct mtk_com_pwm {
	struct pwm_chip chip;
	struct device *dev;
	void __iomem *base;
	struct clk *clks[PWM_NUM_MAX];
	struct clk *clk_main;
	struct clk *clk_top;
	const struct mtk_com_pwm_data *data;
};

static const unsigned long mt8167_pwm_register[PWM_NUM_MAX] = {
	0x0010, 0x0050, 0x0090, 0x00D0, 0x0110,
};

static struct mtk_com_pwm_data mt8167_pwm_data = {
	.pwm_register = &mt8167_pwm_register[0],
	.pwm_nums = 5,
};

static const struct of_device_id pwm_of_match[] = {
	{.compatible = "mediatek,pwm", .data = &mt8167_pwm_data},
	{},
};

static inline struct mtk_com_pwm *to_mtk_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_com_pwm, chip);
}

static int mtk_pwm_is_clk_prepared(struct clk *clk)
{
	int r = 0;
	struct clk_hw *c_hw;

	if (clk) {
		c_hw = __clk_get_hw(clk);
		r = clk_hw_is_prepared(c_hw);
	}

	return r;
}

static unsigned int mtk_pwm_is_clk_enabled(struct clk *clk)
{
	unsigned int r = 0;

	if (clk)
		r = __clk_get_enable_count(clk);

	return r;
}

static int mtk_pwm_clk_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);
	int ret = 0;

	ret = clk_prepare_enable(mt_pwm->clk_top);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(mt_pwm->clk_main);
	if (ret < 0) {
		clk_disable_unprepare(mt_pwm->clk_top);
		return ret;
	}

	ret = clk_prepare_enable(mt_pwm->clks[pwm->hwpwm]);
	if (ret < 0) {
		clk_disable_unprepare(mt_pwm->clk_main);
		clk_disable_unprepare(mt_pwm->clk_top);
		return ret;
	}

	return ret;
}

static void mtk_pwm_clk_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);

	clk_disable_unprepare(mt_pwm->clks[pwm->hwpwm]);
	clk_disable_unprepare(mt_pwm->clk_main);
	clk_disable_unprepare(mt_pwm->clk_top);
}

static inline void mtk_pwm_writel(struct mtk_com_pwm *pwm,
				u32 pwm_no, unsigned long offset,
				unsigned long val)
{
	void __iomem *reg = pwm->base + pwm->data->pwm_register[pwm_no] + offset;

	writel(val, reg);
}

static inline u32 mtk_pwm_readl(struct mtk_com_pwm *pwm,
				u32 pwm_no, unsigned long offset)
{
	u32 value = 0;
	void __iomem *reg = pwm->base + pwm->data->pwm_register[pwm_no] + offset;

	value = readl(reg);
	return value;
}


static int mtk_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			int duty_ns, int period_ns)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);
	u32 value = 0;
	u32 resolution = 100 / 4;
	u32 clkdiv = 0;
	u32 clksrc_rate = 0;

	u32 data_width, thresh;

	mtk_pwm_clk_enable(chip, pwm);

	clksrc_rate = clk_get_rate(mt_pwm->clks[pwm->hwpwm]);
	if (clksrc_rate == 0) {
		PWM_ERR(mt_pwm->dev, "clksrc_rate %d is invalid\n", clksrc_rate);
		return -EINVAL;
	}
	resolution = 1000000000/clksrc_rate;

	while (period_ns / resolution  > 8191) {
		clkdiv++;
		resolution *= 2;
	}

	if (clkdiv > PWM_CLK_DIV_MAX) {
		PWM_ERR(mt_pwm->dev, "period %d not supported\n", period_ns);
		return -EINVAL;
	}

	data_width = period_ns / resolution;
	thresh = duty_ns / resolution;

	value = mtk_pwm_readl(mt_pwm, pwm->hwpwm, PWMCON);
	value = value | BIT(15) | clkdiv;
	mtk_pwm_writel(mt_pwm, pwm->hwpwm, PWMCON, value);

	mtk_pwm_writel(mt_pwm, pwm->hwpwm, PWMDWIDTH, data_width);
	mtk_pwm_writel(mt_pwm, pwm->hwpwm, PWMTHRES, thresh);

	mtk_pwm_clk_disable(chip, pwm);

	return 0;
}

static int mtk_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);
	u32 value;

	pr_info("mtk_pwm_enable +\n");
	mtk_pwm_clk_enable(chip, pwm);

	value = readl(mt_pwm->base + PWM_EN_REG);
	value |= BIT(pwm->hwpwm);
	writel(value, mt_pwm->base + PWM_EN_REG);

	pr_info("PWM_EN_REG:0x%x\n", readl(mt_pwm->base + PWM_EN_REG));
	return 0;
}

static void mtk_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);
	u32 value;

	value = readl(mt_pwm->base + PWM_EN_REG);
	value &= ~BIT(pwm->hwpwm);
	writel(value, mt_pwm->base + PWM_EN_REG);

	mtk_pwm_clk_disable(chip, pwm);
}

#ifdef CONFIG_DEBUG_FS
static void mtk_pwm_dbg_show(struct pwm_chip *chip, struct seq_file *s)
{
	struct mtk_com_pwm *mt_pwm = to_mtk_pwm(chip);
	u32 value;
	int i;

	seq_printf(s, "enable[0x%x]:0x%x, clk(top) prepared:%d, enabled:%u, clk(main) prepared:%d, enabled:%u\n",
			PWM_EN_REG, readl(mt_pwm->base + PWM_EN_REG),
			mtk_pwm_is_clk_prepared(mt_pwm->clk_top),
			mtk_pwm_is_clk_enabled(mt_pwm->clk_top),
			mtk_pwm_is_clk_prepared(mt_pwm->clk_main),
			mtk_pwm_is_clk_enabled(mt_pwm->clk_main));

	for (i = 0; i < mt_pwm->data->pwm_nums; ++i) {
		value = mtk_pwm_readl(mt_pwm, i, PWM_SEND_WAVENUM);
		seq_printf(s, " pwm%d: send wavenum:%u\n", i, value);
		seq_printf(s, "\tenable_bit[%d], clk prepared:%d, enabled:%u\n",
			(readl(mt_pwm->base + PWM_EN_REG) & (1 << i)) ? 1 : 0,
			mtk_pwm_is_clk_prepared(mt_pwm->clks[i]),
			mtk_pwm_is_clk_enabled(mt_pwm->clks[i]));
		seq_printf(s, "\tPWM_CON:0x%x, PWM_DWIDTH:0x%x, PWM_THRESH:0x%x, PWM_CONTINUOUS:%d\n",
			mtk_pwm_readl(mt_pwm, i, PWMCON),
			mtk_pwm_readl(mt_pwm, i, PWMDWIDTH),
			mtk_pwm_readl(mt_pwm, i, PWMTHRES),
			mtk_pwm_readl(mt_pwm, i, PWMWAVENUM) == 0 ? 1 : 0);
	}
}
#endif

static const struct pwm_ops mtk_pwm_ops = {
	.config = mtk_pwm_config,
	.enable = mtk_pwm_enable,
	.disable = mtk_pwm_disable,
#ifdef CONFIG_DEBUG_FS
	.dbg_show = mtk_pwm_dbg_show,
#endif
	.owner = THIS_MODULE,
};

static int mtk_pwm_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct mtk_com_pwm *mt_pwm;
	struct resource *res;
	int ret;
	int i;

	pr_info("[upstream] mtk_pwm_probe +\n");

	id = of_match_device(pwm_of_match, &pdev->dev);
	if (!id)
		return -EINVAL;

	mt_pwm = devm_kzalloc(&pdev->dev, sizeof(*mt_pwm), GFP_KERNEL);
	if (!mt_pwm)
		return -ENOMEM;

	mt_pwm->data = id->data;
	mt_pwm->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mt_pwm->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mt_pwm->base))
		return PTR_ERR(mt_pwm->base);

	for (i = 0; i < mt_pwm->data->pwm_nums; i++) {
		mt_pwm->clks[i] = devm_clk_get(&pdev->dev, mtk_pwm_clk_name[i]);
		if (IS_ERR(mt_pwm->clks[i])) {
			PWM_ERR(&pdev->dev, "[PWM] clock: %s fail\n", mtk_pwm_clk_name[i]);
			return PTR_ERR(mt_pwm->clks[i]);
		}
	}

	mt_pwm->clk_main = devm_clk_get(&pdev->dev, PWM_CLK_NAME_MAIN);
	if (IS_ERR(mt_pwm->clk_main))
		return PTR_ERR(mt_pwm->clk_main);

	mt_pwm->clk_top = devm_clk_get(&pdev->dev, PWM_CLK_NAME_TOP);
	if (IS_ERR(mt_pwm->clk_top))
		return PTR_ERR(mt_pwm->clk_top);

	mt_pwm->chip.dev = &pdev->dev;
	mt_pwm->chip.ops = &mtk_pwm_ops;
	mt_pwm->chip.npwm = mt_pwm->data->pwm_nums;

	platform_set_drvdata(pdev, mt_pwm);

	ret = pwmchip_add(&mt_pwm->chip);
	if (ret < 0) {
		PWM_ERR(&pdev->dev, "---- pwmchip_add() fail, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_pwm_remove(struct platform_device *pdev)
{
	struct mtk_com_pwm *mt_pwm = platform_get_drvdata(pdev);
	int ret;

	ret = pwmchip_remove(&mt_pwm->chip);
	return ret;
}

struct platform_driver mtk_pwm_driver = {
	.probe = mtk_pwm_probe,
	.remove = mtk_pwm_remove,
	.driver = {
		.name = "mtk-pwm",
		.of_match_table = pwm_of_match,
	},
};
MODULE_DEVICE_TABLE(of, pwm_of_match);

module_platform_driver(mtk_pwm_driver);

MODULE_AUTHOR("Zhi Mao <zhi.mao@mediatek.com>");
MODULE_DESCRIPTION("MediaTek SoC PWM driver");
MODULE_LICENSE("GPL v2");


