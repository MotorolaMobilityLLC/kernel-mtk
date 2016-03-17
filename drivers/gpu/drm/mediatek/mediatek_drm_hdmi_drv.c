/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/component.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include "mediatek_hdmi_display_core.h"
#include "mediatek_hdmi.h"
#include "mediatek_hdmi_regs.h"

static const char * const mediatek_hdmi_clk_name[] = {
	"cec",
	"tvdpll",
	"dpi_sel",
	"dpi_div2",
	"dpi_div4",
	"dpi_div8",
	"hdmi_sel",
	"hdmi_div1",
	"hdmi_div2",
	"hdmi_div3",
	"hdmi_pixel",
	"hdmi_pll",
	"dpi_pixel",
	"dpi_enging",
	"aud_bclk",
	"aud_spdif",
};

int mtk_hdmi_get_all_clk(struct mediatek_hdmi *hdmi, struct device_node *np)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mediatek_hdmi_clk_name); i++) {
		hdmi->clk[i] = of_clk_get_by_name(np,
			       mediatek_hdmi_clk_name[i]);
		if (IS_ERR(hdmi->clk[i])) {
			mtk_hdmi_err("get clk %s failed!\n",
				     mediatek_hdmi_clk_name[i]);
			return PTR_ERR(hdmi->clk[i]);
		}
	}
	return 0;
}

int mtk_hdmi_clk_all_enable(struct mediatek_hdmi *hdmi)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(mediatek_hdmi_clk_name); i++) {
		ret = mtk_hdmi_clk_enable(hdmi, i);
		if (ret) {
			mtk_hdmi_err("enable clk %s failed!\n",
				     mediatek_hdmi_clk_name[i]);
			goto errcode;
		}
	}
	return 0;

errcode:
	for (; i; i--)
		mtk_hdmi_clk_disable(hdmi, i - 1);

	return ret;
}

void mtk_hdmi_clk_all_disable(struct mediatek_hdmi *hdmi)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mediatek_hdmi_clk_name); i++)
		mtk_hdmi_clk_disable(hdmi, i);
}

static void hdmi_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static void hdmi_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct mediatek_hdmi *hctx;

	hctx = hdmi_ctx_from_encoder(encoder);
	if (DRM_MODE_DPMS_ON == mode)
		mtk_hdmi_signal_on(hctx);
	else
		mtk_hdmi_signal_off(hctx);
}

static bool hdmi_encoder_mode_fixup(struct drm_encoder *encoder,
				    const struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void hdmi_encoder_mode_set(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct mediatek_hdmi *hdmi_context = NULL;

	hdmi_context = hdmi_ctx_from_encoder(encoder);
	if (!hdmi_context) {
		mtk_hdmi_err("%s failed, invalid hdmi context!\n", __func__);
		return;
	}

	mtk_hdmi_info("cur info: name:%s, hdisplay:%d\n",
		      adjusted_mode->name,
		      adjusted_mode->hdisplay);
	mtk_hdmi_info("hsync_start:%d,hsync_end:%d, htotal:%d",
		      adjusted_mode->hsync_start,
		      adjusted_mode->hsync_end,
		      adjusted_mode->htotal);
	mtk_hdmi_info("hskew:%d, vdisplay:%d\n",
		      adjusted_mode->hskew, adjusted_mode->vdisplay);
	mtk_hdmi_info("vsync_start:%d, vsync_end:%d, vtotal:%d",
		      adjusted_mode->vsync_start,
		      adjusted_mode->vsync_end,
		      adjusted_mode->vtotal);
	mtk_hdmi_info("vscan:%d, flag:%d\n",
		      adjusted_mode->vscan, adjusted_mode->flags);
	mtk_hdmi_display_set_vid_format(hdmi_context->display_node,
					adjusted_mode);

	memcpy(&hdmi_context->display_node->mode, adjusted_mode,
	       sizeof(struct drm_display_mode));
}

static void hdmi_encoder_prepare(struct drm_encoder *encoder)
{
	/* DRM_MODE_DPMS_OFF? */

	/* drm framework doesn't check NULL. */
}

static void hdmi_encoder_commit(struct drm_encoder *encoder)
{
	/* DRM_MODE_DPMS_ON? */
}

static enum drm_connector_status hdmi_conn_detect(struct drm_connector *conn,
						  bool force)
{
	struct mediatek_hdmi *hdmi_context = hdmi_ctx_from_conn(conn);

	if (hdmi_context->hpd)
		return connector_status_connected;
	else
		return connector_status_disconnected;
}

static void hdmi_conn_destroy(struct drm_connector *conn)
{
	drm_connector_cleanup(conn);
}

static int hdmi_conn_set_property(struct drm_connector *conn,
				  struct drm_property *property, uint64_t val)
{
	return 0;
}

static int mtk_hdmi_conn_get_modes(struct drm_connector *conn)
{
	struct mediatek_hdmi *hdmi_context = hdmi_ctx_from_conn(conn);
	struct edid *edid;

	if (!hdmi_context->ddc_adpt)
		return -ENODEV;

	edid = drm_get_edid(conn, hdmi_context->ddc_adpt);
	if (!edid)
		return -ENODEV;

	hdmi_context->dvi_mode = !drm_detect_hdmi_monitor(edid);

	drm_mode_connector_update_edid_property(conn, edid);

	return drm_add_edid_modes(conn, edid);
}

static int mtk_hdmi_conn_mode_valid(struct drm_connector *conn,
				    struct drm_display_mode *mode)
{
	mtk_hdmi_info("xres=%d, yres=%d, refresh=%d, intl=%d clock=%d\n",
		      mode->hdisplay, mode->vdisplay, mode->vrefresh,
		(mode->flags & DRM_MODE_FLAG_INTERLACE) ? true :
		false, mode->clock * 1000);

	if (((mode->clock) >= 27000) || ((mode->clock) <= 297000))
		return MODE_OK;

	return MODE_BAD;
}

static struct drm_encoder *mtk_hdmi_conn_best_enc(struct drm_connector *conn)
{
	struct mediatek_hdmi *hdmi_context = hdmi_ctx_from_conn(conn);

	return &hdmi_context->encoder;
}

static const struct drm_encoder_funcs mtk_hdmi_encoder_funcs = {
	.destroy = hdmi_encoder_destroy,
};

static const struct drm_encoder_helper_funcs mtk_hdmi_encoder_helper_funcs = {
	.dpms = hdmi_encoder_dpms,
	.mode_fixup = hdmi_encoder_mode_fixup,
	.mode_set = hdmi_encoder_mode_set,
	.prepare = hdmi_encoder_prepare,
	.commit = hdmi_encoder_commit,
};

static const struct drm_connector_funcs mtk_hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = hdmi_conn_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = hdmi_conn_destroy,
	.set_property = hdmi_conn_set_property,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
	mtk_hdmi_connector_helper_funcs = {
	.get_modes = mtk_hdmi_conn_get_modes,
	.mode_valid = mtk_hdmi_conn_mode_valid,
	.best_encoder = mtk_hdmi_conn_best_enc,
};

static int mtk_hdmi_create_conn_enc(struct mediatek_hdmi *hdmi_context)
{
	int ret;

	ret =
	    drm_encoder_init(hdmi_context->drm_dev, &hdmi_context->encoder,
			     &mtk_hdmi_encoder_funcs, DRM_MODE_ENCODER_TMDS);
	if (ret) {
		mtk_hdmi_err("drm_encoder_init failed! ret = %d\n", ret);
		goto errcode;
	}
	drm_encoder_helper_add(&hdmi_context->encoder,
			       &mtk_hdmi_encoder_helper_funcs);

	ret =
	    drm_connector_init(hdmi_context->drm_dev, &hdmi_context->conn,
			       &mtk_hdmi_connector_funcs,
			       DRM_MODE_CONNECTOR_HDMIA);
	if (ret) {
		mtk_hdmi_err("drm_connector_init failed! ret = %d\n", ret);
		goto errcode;
	}
	drm_connector_helper_add(&hdmi_context->conn,
				 &mtk_hdmi_connector_helper_funcs);

	hdmi_context->conn.polled = DRM_CONNECTOR_POLL_HPD;
	hdmi_context->conn.interlace_allowed = true;
	hdmi_context->conn.doublescan_allowed = false;

	ret = drm_connector_register(&hdmi_context->conn);
	if (ret) {
		mtk_hdmi_err("drm_connector_register failed! (%d)\n", ret);
		goto errcode;
	}

	ret =
	    drm_mode_connector_attach_encoder(&hdmi_context->conn,
					      &hdmi_context->encoder);
	if (ret) {
		mtk_hdmi_err("drm_mode_connector_attach_encoder failed! (%d)\n",
			     ret);
		goto errcode;
	}

	hdmi_context->conn.encoder = &hdmi_context->encoder;
	hdmi_context->encoder.possible_crtcs = 0x2;

	mtk_hdmi_info("create encoder and connector success!\n");

	return 0;

errcode:
	return ret;
}

static void mtk_hdmi_destroy_conn_enc(struct mediatek_hdmi
				      *hdmi_context)
{
	if (hdmi_context == NULL)
		return;

	drm_encoder_cleanup(&hdmi_context->encoder);
	drm_connector_unregister(&hdmi_context->conn);
	drm_connector_cleanup(&hdmi_context->conn);
}

static struct mediatek_hdmi_display_ops hdmi_display_ops = {
	.init = mtk_hdmi_output_init,
	.set_format = mtk_hdmi_output_set_display_mode,
};

static int mtk_hdmi_bind(struct device *dev, struct device *master, void *data)
{
	struct device_node *prenode;
	struct mediatek_hdmi *hdmi_context = NULL;
	struct mediatek_hdmi_display_node *display_node = NULL;

	hdmi_context = platform_get_drvdata(to_platform_device(dev));
	if (!hdmi_context) {
		mtk_hdmi_err(" platform_get_drvdata failed!\n");
		return -EFAULT;
	}

	prenode = of_parse_phandle(dev->of_node, "prenode", 0);
	if (!prenode) {
		mtk_hdmi_err("find prenode node failed!\n");
		return -EFAULT;
	}

	display_node = mtk_hdmi_display_find_node(prenode);
	if (!display_node) {
		mtk_hdmi_err("find display node failed!\n");
		return -EFAULT;
	}

	if (mtk_hdmi_display_add_pre_node
	    (hdmi_context->display_node, display_node)) {
		mtk_hdmi_err("add display node failed!\n");
		return -EFAULT;
	}

	if (!mtk_hdmi_display_init(hdmi_context->display_node))
		mtk_hdmi_err("init display failed!\n");

	hdmi_context->audio_pdev_info.parent = dev;
	hdmi_context->audio_pdev_info.id = PLATFORM_DEVID_NONE;
	hdmi_context->audio_pdev_info.name = "mediatek,mt2701-hdmi-audio";
	hdmi_context->audio_pdev_info.dma_mask = DMA_BIT_MASK(32);

	hdmi_context->audio_data.irq = hdmi_context->irq;
	hdmi_context->audio_data.mediatek_hdmi = hdmi_context;
	hdmi_context->audio_data.enable = mtk_hdmi_audio_enable;
	hdmi_context->audio_data.disable = mtk_hdmi_audio_disabe;
	hdmi_context->audio_data.set_audio_param = mtk_hdmi_audio_set_param;
	hdmi_context->audio_data.hpd_detect = mtk_hdmi_hpd_high;
	hdmi_context->audio_data.detect_dvi_monitor =
		mtk_hdmi_detect_dvi_monitor;

	hdmi_context->audio_pdev_info.data =
		&hdmi_context->audio_data;
	hdmi_context->audio_pdev_info.size_data =
		sizeof(hdmi_context->audio_data);
	hdmi_context->audio_pdev =
		platform_device_register_full(&hdmi_context->audio_pdev_info);
	hdmi_context->drm_dev = data;
	mtk_hdmi_info(" hdmi_context = %p, data = %p , display_node = %p\n",
		      hdmi_context, data, display_node);

	return mtk_hdmi_create_conn_enc(hdmi_context);
}

static void mtk_hdmi_unbind(struct device *dev,
			    struct device *master, void *data)
{
	struct mediatek_hdmi *hdmi_context = NULL;

	hdmi_context = platform_get_drvdata(to_platform_device(dev));
	mtk_hdmi_destroy_conn_enc(hdmi_context);

	platform_device_unregister(hdmi_context->audio_pdev);

	hdmi_context->drm_dev = NULL;
}

static const struct component_ops mediatek_hdmi_component_ops = {
	.bind = mtk_hdmi_bind,
	.unbind = mtk_hdmi_unbind,
};

static void __iomem *mtk_hdmi_resource_ioremap(struct platform_device *pdev,
					       u32 index)
{
	struct resource *mem;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, index);
	if (!mem) {
		mtk_hdmi_err("get memory source fail!\n");
		return ERR_PTR(-EFAULT);
	}

	mtk_hdmi_info("index :%d , physical adr: 0x%llx, end: 0x%llx\n", index,
		      mem->start, mem->end);

	return devm_ioremap_resource(&pdev->dev, mem);
}

static int mtk_hdmi_dt_parse_pdata(struct mediatek_hdmi *hdmi_context,
				   struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *i2c_np = NULL;
	int ret;

	hdmi_context->flt_n_5v_gpio = of_get_named_gpio(np,
							"flt_n_5v-gpios",
							0);
	if (hdmi_context->flt_n_5v_gpio < 0) {
		mtk_hdmi_err
		    ("hdmi_context->flt_n_5v_gpio = %d\n",
		     hdmi_context->flt_n_5v_gpio);
		goto errcode;
	}

	ret = mtk_hdmi_get_all_clk(hdmi_context, np);
	if (ret) {
		mtk_hdmi_err("get clk from dt failed!\n");
		goto errcode;
	}

	ret = mtk_hdmi_clk_all_enable(hdmi_context);
	if (ret) {
		mtk_hdmi_err("enable all clk failed!\n");
		goto errcode;
	}

	hdmi_context->sys_regs = mtk_hdmi_resource_ioremap(pdev, 0);
	if (IS_ERR(hdmi_context->sys_regs)) {
		mtk_hdmi_err(" mem resource ioremap failed\n");
		goto errcode;
	}

	hdmi_context->pll_regs = mtk_hdmi_resource_ioremap(pdev, 1);
	if (IS_ERR(hdmi_context->pll_regs)) {
		mtk_hdmi_err(" mem resource ioremap failed!\n");
		goto errcode;
	}

	hdmi_context->grl_regs = mtk_hdmi_resource_ioremap(pdev, 2);
	if (IS_ERR(hdmi_context->grl_regs)) {
		mtk_hdmi_err(" mem resource ioremap failed!\n");
		goto errcode;
	}

	hdmi_context->cec_regs = mtk_hdmi_resource_ioremap(pdev, 3);
	if (IS_ERR(hdmi_context->cec_regs)) {
		mtk_hdmi_err(" mem resource ioremap failed!\n");
		goto errcode;
	}

	hdmi_context->irq = irq_of_parse_and_map(np, 0);
	if (hdmi_context->irq < 0) {
		mtk_hdmi_err(" get irq failed, irq = %d !\n",
			     hdmi_context->irq);
		goto errcode;
	}

	i2c_np = of_parse_phandle(np, "i2cnode", 0);
	if (!i2c_np) {
		mtk_hdmi_err("find i2c node failed!\n");
		goto errcode;
	}

	hdmi_context->ddc_adpt = of_find_i2c_adapter_by_node(i2c_np);
	if (!hdmi_context->ddc_adpt) {
		mtk_hdmi_err("Failed to get ddc i2c adapter by node\n");
		goto errcode;
	}

	mtk_hdmi_info("irq:%d\n", hdmi_context->irq);
	mtk_hdmi_info("hdmi_context->ddc_adpt :0x%p\n", hdmi_context->ddc_adpt);
	mtk_hdmi_info("hdmi_context->flt_n_5v_gpio :%d\n",
		      hdmi_context->flt_n_5v_gpio);
	mtk_hdmi_info("set htplg pinctrl success!");

	return 0;

errcode:
	return -ENXIO;
}

static irqreturn_t hdmi_flt_n_5v_irq_thread(int irq, void *arg)
{
	mtk_hdmi_err("detected 5v pin error status\n");
	return IRQ_HANDLED;
}

static irqreturn_t hdmi_htplg_isr_thread(int irq, void *arg)
{
	struct mediatek_hdmi *hdmi_context = arg;
	bool hpd;

	mtk_hdmi_htplg_irq_clr(hdmi_context);
	hpd = mtk_hdmi_hpd_high(hdmi_context);

	if (hdmi_context->hpd != hpd) {
		mtk_hdmi_info("hotplug event!,cur hpd = %d, hpd = %d\n",
			      hdmi_context->hpd, hpd);
		hdmi_context->hpd = hpd;
		if (hdmi_context->drm_dev)
			drm_helper_hpd_irq_event(hdmi_context->drm_dev);
	}
	return IRQ_HANDLED;
}

static int mtk_drm_hdmi_probe(struct platform_device *pdev)
{
	int ret;
	struct mediatek_hdmi *hctx = NULL;
	struct device *dev = &pdev->dev;

	hctx = devm_kzalloc(dev, sizeof(*hctx), GFP_KERNEL);
	if (!hctx)
		return -ENOMEM;

	ret = mtk_hdmi_dt_parse_pdata(hctx, pdev);
	if (ret) {
		mtk_hdmi_err("mtk_hdmi_dt_parse_pdata failed!!\n");
		return ret;
	}

	hctx->flt_n_5v_irq = gpio_to_irq(hctx->flt_n_5v_gpio);
	if (hctx->flt_n_5v_irq < 0) {
		mtk_hdmi_err("hdmi_context->flt_n_5v_irq = %d\n",
			     hctx->flt_n_5v_irq);
		return hctx->flt_n_5v_irq;
	}

	ret = devm_request_threaded_irq(dev, hctx->flt_n_5v_irq,
					NULL, hdmi_flt_n_5v_irq_thread,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"hdmi flt_n_5v", hctx);
	if (ret) {
		mtk_hdmi_err("failed to register hdmi flt_n_5v interrupt\n");
		return ret;
	}

	ret = devm_request_threaded_irq(dev, hctx->irq,
					NULL, hdmi_htplg_isr_thread,
					IRQF_SHARED | IRQF_TRIGGER_LOW |
					IRQF_ONESHOT,
					"hdmi hpd", hctx);
	if (ret) {
		mtk_hdmi_err("failed to register hdmi hpd interrupt\n");
		return ret;
	}

	hctx->display_node = mtk_hdmi_display_create_node(&hdmi_display_ops,
							  pdev->dev.of_node,
							  hctx);
	if (IS_ERR(hctx->display_node)) {
		mtk_hdmi_err("mtk_hdmi_display_create_node failed!\n");
		return PTR_ERR(hctx->display_node);
	}

	platform_set_drvdata(pdev, hctx);
	mutex_init(&hctx->hdmi_mutex);

	ret = mtk_drm_hdmi_debugfs_init(hctx);
	if (ret) {
		mtk_hdmi_err("init debugfs failed!\n");
		mtk_hdmi_display_del_node(hctx->display_node);
		return ret;
	}

	ret = component_add(&pdev->dev, &mediatek_hdmi_component_ops);
	if (ret) {
		mtk_hdmi_err("component_add failed !\n");
		mtk_drm_hdmi_debugfs_exit(hctx);
		mtk_hdmi_display_del_node(hctx->display_node);
		return ret;
	}

	return 0;
}

static int mtk_drm_hdmi_remove(struct platform_device *pdev)
{
	struct mediatek_hdmi *hdmi_context = NULL;

	hdmi_context = platform_get_drvdata(pdev);

	component_del(&pdev->dev, &mediatek_hdmi_component_ops);
	platform_set_drvdata(pdev, NULL);
	mtk_drm_hdmi_debugfs_exit(hdmi_context);

	if (hdmi_context != NULL)
		kfree(hdmi_context);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_hdmi_suspend(struct device *dev)
{
	struct mediatek_hdmi *hctx = NULL;

	hctx = dev_get_drvdata(dev);
	if (IS_ERR(hctx)) {
		mtk_hdmi_info("hdmi suspend failed!\n");
		return PTR_ERR(hctx);
	}

	mtk_hdmi_clk_all_disable(hctx);
	mtk_hdmi_info("hdmi suspend success!\n");
	return 0;
}

static int mtk_hdmi_resume(struct device *dev)
{
	struct mediatek_hdmi *hctx = NULL;
	int ret = 0;

	hctx = dev_get_drvdata(dev);
	if (IS_ERR(hctx)) {
		mtk_hdmi_info("hdmi suspend failed!\n");
		return PTR_ERR(hctx);
	}

	ret = mtk_hdmi_clk_all_enable(hctx);
	if (ret) {
		mtk_hdmi_info("hdmi suspend failed!\n");
		return ret;
	}

	mtk_hdmi_display_set_vid_format(hctx->display_node,
					&hctx->display_node->mode);
	mtk_hdmi_info("hdmi resume success!\n");
	return 0;
}

static const struct dev_pm_ops mediatek_hdmi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_hdmi_suspend, mtk_hdmi_resume)
};

#endif

static const struct of_device_id mediatek_drm_hdmi_of_ids[] = {
	{.compatible = "mediatek,mt2701-hdmi",},
	{}
};

struct platform_driver mediatek_hdmi_platform_driver = {
	.probe = mtk_drm_hdmi_probe,
	.remove = mtk_drm_hdmi_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mediatek-drm-hdmi",
		   .of_match_table = mediatek_drm_hdmi_of_ids,
#ifdef CONFIG_PM_SLEEP
		   .pm = &mediatek_hdmi_pm_ops,
#endif
		   },
};

int mtk_hdmitx_init(void)
{
	int ret;

	ret = platform_driver_register(&mediatek_hdmi_ddc_driver);
	if (ret < 0)
		mtk_hdmi_err("register hdmiddc platform driver failed!");

	ret = platform_driver_register(&mediate_hdmi_dpi_driver);
	if (ret < 0)
		mtk_hdmi_err("register hdmidpi platform driver failed!");

	ret = platform_driver_register(&mediatek_hdmi_platform_driver);
	if (ret < 0)
		mtk_hdmi_err("register hdmitx platform driver failed!");

	return 0;
}

module_init(mtk_hdmitx_init);

MODULE_AUTHOR("Jie Qiu <jie.qiu@mediatek.com>");
MODULE_DESCRIPTION("MediaTek HDMI Driver");
MODULE_LICENSE("GPL");
