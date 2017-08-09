#include <linux/slab.h>

#include <linux/types.h>
#include "disp_drv_log.h"
#include "lcm_drv.h"
#include "disp_drv_platform.h"
#include "ddp_manager.h"
#include "mtkfb.h"

#if !defined(CONFIG_MTK_LEGACY)
#include <linux/of.h>
/* extern unsigned int lcm_driver_id; */
/* extern unsigned int lcm_module_id; */
#define MAX_INIT_CNT 256
#define REGFLAG_DELAY 0xFE
#endif

#include "disp_lcm.h"

int _lcm_count(void)
{
	return lcm_count;
}

int _is_lcm_inited(disp_lcm_handle *plcm)
{
	if (plcm) {
		if (plcm->params && plcm->drv)
			return 1;
	} else {
		DISPERR("WARNING, invalid lcm handle: %p\n", plcm);
		return 0;
	}

	return 0;
}

LCM_PARAMS *_get_lcm_params_by_handle(disp_lcm_handle *plcm)
{
	if (plcm)
		return plcm->params;

	DISPERR("WARNING, invalid lcm handle:%p\n", plcm);
	return NULL;
}

LCM_DRIVER *_get_lcm_driver_by_handle(disp_lcm_handle *plcm)
{
	if (plcm)
		return plcm->drv;

	DISPERR("WARNING, invalid lcm handle:%p\n", plcm);
	return NULL;
}

void _dump_lcm_info(disp_lcm_handle *plcm)
{
	LCM_DRIVER *l = NULL;
	LCM_PARAMS *p = NULL;
	char str_buf[128];
	int buf_len = 128;
	int buf_pos = 0;

	if (plcm == NULL) {
		DISPERR("plcm is null\n");
		return;
	}

	l = plcm->drv;
	p = plcm->params;

	if (l && p) {
		buf_pos += scnprintf(str_buf + buf_pos, 128 - buf_pos, "resolution: %d x %d",
				     p->width, p->height);

		switch (p->lcm_if) {
		case LCM_INTERFACE_DSI0:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: DSI0");
			break;
		case LCM_INTERFACE_DSI1:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: DSI1");
			break;
		case LCM_INTERFACE_DPI0:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: DPI0");
			break;
		case LCM_INTERFACE_DPI1:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: DPI1");
			break;
		case LCM_INTERFACE_DBI0:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: DBI0");
			break;
		default:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", interface: unknown");
			break;
		}

		switch (p->type) {
		case LCM_TYPE_DBI:
			buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", Type : DBI");
			break;
		case LCM_TYPE_DSI:
			buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", Type : DSI");
			break;
		case LCM_TYPE_DPI:
			buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", Type : DPI");
			break;
		default:
			buf_pos +=
			    scnprintf(str_buf + buf_pos, buf_len - buf_pos, ", Type : unknown");
			break;
		}

		if (p->type == LCM_TYPE_DSI) {
			switch (p->dsi.mode) {
			case CMD_MODE:
				buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos,
						     ", DSI Mode: CMD_MODE");
				break;
			case SYNC_PULSE_VDO_MODE:
				buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos,
						     ", DSI Mode: SYNC_PULSE_VDO_MODE");
				break;
			case SYNC_EVENT_VDO_MODE:
				buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos,
						     ", DSI Mode: SYNC_EVENT_VDO_MODE");
				break;
			case BURST_VDO_MODE:
				buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos,
						     ", DSI Mode: BURST_VDO_MODE");
				break;
			default:
				buf_pos += scnprintf(str_buf + buf_pos, buf_len - buf_pos,
						     ", DSI Mode: Unknown");
				break;
			}
		}
#if 0
		if (p->type == LCM_TYPE_DSI) {
			DISPCHECK("[LCM] LANE_NUM: %d,data_format\n", (int)p->dsi.LANE_NUM);
#ifdef MT_TODO
#error
#endif
			DISPCHECK("[LCM] vact: %u, vbp: %u, vfp: %u,\n",
				  p->dsi.vertical_sync_active, p->dsi.vertical_backporch, p->dsi.vertical_frontporch);
			DISPCHECK("vact_line: %u, hact: %u, hbp: %u, hfp: %u, hblank: %u\n",
				  p->dsi.vertical_active_line, p->dsi.horizontal_sync_active,
				  p->dsi.horizontal_backporch, p->dsi.horizontal_frontporch,
				  p->dsi.horizontal_blanking_pixel);
			DISPCHECK("[LCM] pll_select: %d, pll_div1: %d, pll_div2: %d, fbk_div: %d,\n",
				  p->dsi.pll_select, p->dsi.pll_div1, p->dsi.pll_div2, p->dsi.fbk_div);
			DISPCHECK("fbk_sel: %d, rg_bir: %d\n", p->dsi.fbk_sel, p->dsi.rg_bir);
			DISPCHECK("[LCM] rg_bic: %d, rg_bp: %d, PLL_CLOCK: %d, dsi_clock: %d,\n",
				  p->dsi.rg_bic, p->dsi.rg_bp, p->dsi.PLL_CLOCK, p->dsi.dsi_clock);
			DISPCHECK("ssc_range: %d, ssc_disable: %d, compatibility_for_nvk: %d, cont_clock: %d\n",
				  p->dsi.ssc_range, p->dsi.ssc_disable,
				  p->dsi.compatibility_for_nvk, p->dsi.cont_clock);
			DISPCHECK("[LCM] lcm_ext_te_enable: %d, noncont_clock: %d, noncont_clock_period: %d\n",
				  p->dsi.lcm_ext_te_enable, p->dsi.noncont_clock, p->dsi.noncont_clock_period);
		}
#endif
		DISPPRINT("[LCM] %s\n", str_buf);
	}
}

#if !defined(CONFIG_MTK_LEGACY)
struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

void disp_of_getprop_u32(const struct device_node *np, const char *propname, u32 *out_value)
{
	int ret = 0;
	u32 data = 0;

	ret = of_property_read_u32(np, propname, &data);
	/* Fall back to default value 0 on failure */
	*out_value = (ret ? 0 : data);
}

void disp_of_getprop_u8(const struct device_node *np, const char *propname,
			u8 *out_value, size_t out_value_sz)
{
	int ret = 0;

	ret = of_property_read_u8_array(np, propname, out_value, out_value_sz);
	/* Fall back to default value 0 on failure */
	if (ret)
		memset(out_value, 0, out_value_sz);
}

void parse_lcm_params_dt_node(struct device_node *np, LCM_PARAMS *lcm_params)
{
	if (!lcm_params) {
		pr_err("%s:%d, ERROR: Error access to LCM_PARAMS(NULL)\n", __FILE__, __LINE__);
		return;
	}

	disp_of_getprop_u32(np, "lcm_params-type", &lcm_params->type);
	disp_of_getprop_u32(np, "lcm_params-ctrl", &lcm_params->ctrl);
	disp_of_getprop_u32(np, "lcm_params-lcm_if", &lcm_params->lcm_if);
	disp_of_getprop_u32(np, "lcm_params-lcm_cmd_if", &lcm_params->lcm_cmd_if);
	disp_of_getprop_u32(np, "lcm_params-width", &lcm_params->width);
	disp_of_getprop_u32(np, "lcm_params-height", &lcm_params->height);
	disp_of_getprop_u32(np, "lcm_params-io_select_mode", &lcm_params->io_select_mode);
	disp_of_getprop_u32(np, "lcm_params-dbi-port", &lcm_params->dbi.port);
	disp_of_getprop_u32(np, "lcm_params-dbi-clock_freq", &lcm_params->dbi.clock_freq);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_width", &lcm_params->dbi.data_width);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_format-color_order",
			    &lcm_params->dbi.data_format.color_order);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_format-trans_seq",
			    &lcm_params->dbi.data_format.trans_seq);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_format-padding",
			    &lcm_params->dbi.data_format.padding);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_format-format",
			    &lcm_params->dbi.data_format.format);
	disp_of_getprop_u32(np, "lcm_params-dbi-data_format-width",
			    &lcm_params->dbi.data_format.width);
	disp_of_getprop_u32(np, "lcm_params-dbi-cpu_write_bits", &lcm_params->dbi.cpu_write_bits);
	disp_of_getprop_u32(np, "lcm_params-dbi-io_driving_current",
			    &lcm_params->dbi.io_driving_current);
	disp_of_getprop_u32(np, "lcm_params-dbi-msb_io_driving_current",
			    &lcm_params->dbi.msb_io_driving_current);
	disp_of_getprop_u32(np, "lcm_params-dbi-te_mode", &lcm_params->dbi.te_mode);
	disp_of_getprop_u32(np, "lcm_params-dbi-te_edge_polarity",
			    &lcm_params->dbi.te_edge_polarity);
	disp_of_getprop_u32(np, "lcm_params-dbi-te_hs_delay_cnt", &lcm_params->dbi.te_hs_delay_cnt);
	disp_of_getprop_u32(np, "lcm_params-dbi-te_vs_width_cnt", &lcm_params->dbi.te_vs_width_cnt);
	disp_of_getprop_u32(np, "lcm_params-dbi-te_vs_width_cnt_div",
			    &lcm_params->dbi.te_vs_width_cnt_div);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-cs_polarity",
			    &lcm_params->dbi.serial.cs_polarity);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-clk_polarity",
			    &lcm_params->dbi.serial.clk_polarity);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-clk_phase",
			    &lcm_params->dbi.serial.clk_phase);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-is_non_dbi_mode",
			    &lcm_params->dbi.serial.is_non_dbi_mode);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-clock_base",
			    &lcm_params->dbi.serial.clock_base);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-clock_div",
			    &lcm_params->dbi.serial.clock_div);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-css", &lcm_params->dbi.serial.css);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-csh", &lcm_params->dbi.serial.csh);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-rd_1st", &lcm_params->dbi.serial.rd_1st);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-rd_2nd", &lcm_params->dbi.serial.rd_2nd);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-wr_1st", &lcm_params->dbi.serial.wr_1st);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-wr_2nd", &lcm_params->dbi.serial.wr_2nd);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_3wire",
			    &lcm_params->dbi.serial.sif_3wire);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_sdi", &lcm_params->dbi.serial.sif_sdi);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_1st_pol",
			    &lcm_params->dbi.serial.sif_1st_pol);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_sck_def",
			    &lcm_params->dbi.serial.sif_sck_def);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_div2", &lcm_params->dbi.serial.sif_div2);
	disp_of_getprop_u32(np, "lcm_params-dbi-serial-sif_hw_cs",
			    &lcm_params->dbi.serial.sif_hw_cs);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-write_setup",
			    &lcm_params->dbi.parallel.write_setup);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-write_hold",
			    &lcm_params->dbi.parallel.write_hold);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-write_wait",
			    &lcm_params->dbi.parallel.write_wait);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-read_setup",
			    &lcm_params->dbi.parallel.read_setup);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-read_hold",
			    &lcm_params->dbi.parallel.read_hold);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-read_latency",
			    &lcm_params->dbi.parallel.read_latency);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-wait_period",
			    &lcm_params->dbi.parallel.wait_period);
	disp_of_getprop_u32(np, "lcm_params-dbi-parallel-cs_high_width",
			    &lcm_params->dbi.parallel.cs_high_width);
	disp_of_getprop_u32(np, "lcm_params-dpi-mipi_pll_clk_ref",
			    &lcm_params->dpi.mipi_pll_clk_ref);
	disp_of_getprop_u32(np, "lcm_params-dpi-mipi_pll_clk_div1",
			    &lcm_params->dpi.mipi_pll_clk_div1);
	disp_of_getprop_u32(np, "lcm_params-dpi-mipi_pll_clk_div2",
			    &lcm_params->dpi.mipi_pll_clk_div2);
	disp_of_getprop_u32(np, "lcm_params-dpi-mipi_pll_clk_fbk_div",
			    &lcm_params->dpi.mipi_pll_clk_fbk_div);
	disp_of_getprop_u32(np, "lcm_params-dpi-dpi_clk_div", &lcm_params->dpi.dpi_clk_div);
	disp_of_getprop_u32(np, "lcm_params-dpi-dpi_clk_duty", &lcm_params->dpi.dpi_clk_duty);
	disp_of_getprop_u32(np, "lcm_params-dpi-PLL_CLOCK", &lcm_params->dpi.PLL_CLOCK);
	disp_of_getprop_u32(np, "lcm_params-dpi-dpi_clock", &lcm_params->dpi.dpi_clock);
	disp_of_getprop_u32(np, "lcm_params-dpi-ssc_disable", &lcm_params->dpi.ssc_disable);
	disp_of_getprop_u32(np, "lcm_params-dpi-ssc_range", &lcm_params->dpi.ssc_range);
	disp_of_getprop_u32(np, "lcm_params-dpi-width", &lcm_params->dpi.width);
	disp_of_getprop_u32(np, "lcm_params-dpi-height", &lcm_params->dpi.height);
	disp_of_getprop_u32(np, "lcm_params-dpi-bg_width", &lcm_params->dpi.bg_width);
	disp_of_getprop_u32(np, "lcm_params-dpi-bg_height", &lcm_params->dpi.bg_height);
	disp_of_getprop_u32(np, "lcm_params-dpi-clk_pol", &lcm_params->dpi.clk_pol);
	disp_of_getprop_u32(np, "lcm_params-dpi-de_pol", &lcm_params->dpi.de_pol);
	disp_of_getprop_u32(np, "lcm_params-dpi-vsync_pol", &lcm_params->dpi.vsync_pol);
	disp_of_getprop_u32(np, "lcm_params-dpi-hsync_pol", &lcm_params->dpi.hsync_pol);
	disp_of_getprop_u32(np, "lcm_params-dpi-hsync_pulse_width",
			    &lcm_params->dpi.hsync_pulse_width);
	disp_of_getprop_u32(np, "lcm_params-dpi-hsync_back_porch",
			    &lcm_params->dpi.hsync_back_porch);
	disp_of_getprop_u32(np, "lcm_params-dpi-hsync_front_porch",
			    &lcm_params->dpi.hsync_front_porch);
	disp_of_getprop_u32(np, "lcm_params-dpi-vsync_pulse_width",
			    &lcm_params->dpi.vsync_pulse_width);
	disp_of_getprop_u32(np, "lcm_params-dpi-vsync_back_porch",
			    &lcm_params->dpi.vsync_back_porch);
	disp_of_getprop_u32(np, "lcm_params-dpi-vsync_front_porch",
			    &lcm_params->dpi.vsync_front_porch);
	disp_of_getprop_u32(np, "lcm_params-dpi-format", &lcm_params->dpi.format);
	disp_of_getprop_u32(np, "lcm_params-dpi-rgb_order", &lcm_params->dpi.rgb_order);
	disp_of_getprop_u32(np, "lcm_params-dpi-is_serial_output",
			    &lcm_params->dpi.is_serial_output);
	disp_of_getprop_u32(np, "lcm_params-dpi-i2x_en", &lcm_params->dpi.i2x_en);
	disp_of_getprop_u32(np, "lcm_params-dpi-i2x_edge", &lcm_params->dpi.i2x_edge);
	disp_of_getprop_u32(np, "lcm_params-dpi-embsync", &lcm_params->dpi.embsync);
	disp_of_getprop_u32(np, "lcm_params-dpi-lvds_tx_en", &lcm_params->dpi.lvds_tx_en);
	disp_of_getprop_u32(np, "lcm_params-dpi-intermediat_buffer_num",
			    &lcm_params->dpi.intermediat_buffer_num);
	disp_of_getprop_u32(np, "lcm_params-dpi-io_driving_current",
			    &lcm_params->dpi.io_driving_current);
	disp_of_getprop_u32(np, "lcm_params-dpi-lsb_io_driving_current",
			    &lcm_params->dpi.lsb_io_driving_current);
	disp_of_getprop_u32(np, "lcm_params-dsi-mode", &lcm_params->dsi.mode);
	disp_of_getprop_u32(np, "lcm_params-dsi-switch_mode", &lcm_params->dsi.switch_mode);
	disp_of_getprop_u32(np, "lcm_params-dsi-DSI_WMEM_CONTI", &lcm_params->dsi.DSI_WMEM_CONTI);
	disp_of_getprop_u32(np, "lcm_params-dsi-DSI_RMEM_CONTI", &lcm_params->dsi.DSI_RMEM_CONTI);
	disp_of_getprop_u32(np, "lcm_params-dsi-VC_NUM", &lcm_params->dsi.VC_NUM);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_num", &lcm_params->dsi.LANE_NUM);
	disp_of_getprop_u32(np, "lcm_params-dsi-data_format-color_order",
			    &lcm_params->dsi.data_format.color_order);
	disp_of_getprop_u32(np, "lcm_params-dsi-data_format-trans_seq",
			    &lcm_params->dsi.data_format.trans_seq);
	disp_of_getprop_u32(np, "lcm_params-dsi-data_format-padding",
			    &lcm_params->dsi.data_format.padding);
	disp_of_getprop_u32(np, "lcm_params-dsi-data_format-format",
			    &lcm_params->dsi.data_format.format);
	disp_of_getprop_u32(np, "lcm_params-dsi-intermediat_buffer_num",
			    &lcm_params->dsi.intermediat_buffer_num);
	disp_of_getprop_u32(np, "lcm_params-dsi-ps", &lcm_params->dsi.PS);
	disp_of_getprop_u32(np, "lcm_params-dsi-word_count", &lcm_params->dsi.word_count);
	disp_of_getprop_u32(np, "lcm_params-dsi-packet_size", &lcm_params->dsi.packet_size);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_sync_active",
			    &lcm_params->dsi.vertical_sync_active);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_backporch",
			    &lcm_params->dsi.vertical_backporch);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_frontporch",
			    &lcm_params->dsi.vertical_frontporch);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_frontporch_for_low_power",
			    &lcm_params->dsi.vertical_frontporch_for_low_power);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_active_line",
			    &lcm_params->dsi.vertical_active_line);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_sync_active",
			    &lcm_params->dsi.horizontal_sync_active);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_backporch",
			    &lcm_params->dsi.horizontal_backporch);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_frontporch",
			    &lcm_params->dsi.horizontal_frontporch);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_blanking_pixel",
			    &lcm_params->dsi.horizontal_blanking_pixel);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_active_pixel",
			    &lcm_params->dsi.horizontal_active_pixel);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_bllp", &lcm_params->dsi.horizontal_bllp);
	disp_of_getprop_u32(np, "lcm_params-dsi-line_byte", &lcm_params->dsi.line_byte);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_sync_active_byte",
			    &lcm_params->dsi.horizontal_sync_active_byte);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_backportch_byte",
			    &lcm_params->dsi.horizontal_backporch_byte);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_frontporch_byte",
			    &lcm_params->dsi.horizontal_frontporch_byte);
	disp_of_getprop_u32(np, "lcm_params-dsi-rgb_byte", &lcm_params->dsi.rgb_byte);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_sync_active_word_count",
			    &lcm_params->dsi.horizontal_sync_active_word_count);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_backporch_word_count",
			    &lcm_params->dsi.horizontal_backporch_word_count);
	disp_of_getprop_u32(np, "lcm_params-dsi-horizontal_frontporch_word_count",
			    &lcm_params->dsi.horizontal_frontporch_word_count);

	disp_of_getprop_u8(np, "lcm_params-dsi-HS_TRAIL", &lcm_params->dsi.HS_TRAIL, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-ZERO", &lcm_params->dsi.HS_ZERO, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-HS_PRPR", &lcm_params->dsi.HS_PRPR, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-LPX", &lcm_params->dsi.LPX, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-TA_SACK", &lcm_params->dsi.TA_SACK, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-TA_GET", &lcm_params->dsi.TA_GET, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-TA_SURE", &lcm_params->dsi.TA_SURE, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-TA_GO", &lcm_params->dsi.TA_GO, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CLK_TRAIL", &lcm_params->dsi.CLK_TRAIL, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CLK_ZERO", &lcm_params->dsi.CLK_ZERO, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-LPX_WAIT", &lcm_params->dsi.LPX_WAIT, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CONT_DET", &lcm_params->dsi.CONT_DET, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CLK_HS_PRPR", &lcm_params->dsi.CLK_HS_PRPR, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CLK_HS_POST", &lcm_params->dsi.CLK_HS_POST, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-DA_HS_EXIT", &lcm_params->dsi.DA_HS_EXIT, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-CLK_HS_EXIT", &lcm_params->dsi.CLK_HS_EXIT, 1);

	disp_of_getprop_u32(np, "lcm_params-dsi-pll_select", &lcm_params->dsi.pll_select);
	disp_of_getprop_u32(np, "lcm_params-dsi-pll_div1", &lcm_params->dsi.pll_div1);
	disp_of_getprop_u32(np, "lcm_params-dsi-pll_div2", &lcm_params->dsi.pll_div2);
	disp_of_getprop_u32(np, "lcm_params-dsi-fbk_div", &lcm_params->dsi.fbk_div);
	disp_of_getprop_u32(np, "lcm_params-dsi-fbk_sel", &lcm_params->dsi.fbk_sel);
	disp_of_getprop_u32(np, "lcm_params-dsi-rg_bir", &lcm_params->dsi.rg_bir);
	disp_of_getprop_u32(np, "lcm_params-dsi-rg_bic", &lcm_params->dsi.rg_bic);
	disp_of_getprop_u32(np, "lcm_params-dsi-rg_bp", &lcm_params->dsi.rg_bp);
	disp_of_getprop_u32(np, "lcm_params-dsi-pll_clock", &lcm_params->dsi.PLL_CLOCK);
	disp_of_getprop_u32(np, "lcm_params-dsi-dsi_clock", &lcm_params->dsi.dsi_clock);
	disp_of_getprop_u32(np, "lcm_params-dsi-ssc_disable", &lcm_params->dsi.ssc_disable);
	disp_of_getprop_u32(np, "lcm_params-dsi-ssc_range", &lcm_params->dsi.ssc_range);
	disp_of_getprop_u32(np, "lcm_params-dsi-compatibility_for_nvk",
			    &lcm_params->dsi.compatibility_for_nvk);
	disp_of_getprop_u32(np, "lcm_params-dsi-cont_clock", &lcm_params->dsi.cont_clock);
	disp_of_getprop_u32(np, "lcm_params-dsi-ufoe_enable", &lcm_params->dsi.ufoe_enable);
	disp_of_getprop_u32(np, "lcm_params-dsi-ufoe_params.compress_ratio",
			    &lcm_params->dsi.ufoe_params.compress_ratio);
	disp_of_getprop_u32(np, "lcm_params-dsi-ufoe_params.lr_mode_en",
			    &lcm_params->dsi.ufoe_params.lr_mode_en);
	disp_of_getprop_u32(np, "lcm_params-dsi-ufoe_params.vlc_disable",
			    &lcm_params->dsi.ufoe_params.vlc_disable);
	disp_of_getprop_u32(np, "lcm_params-dsi-ufoe_params.vlc_config",
			    &lcm_params->dsi.ufoe_params.vlc_config);
	disp_of_getprop_u32(np, "lcm_params-dsi-edp_panel", &lcm_params->dsi.edp_panel);
	disp_of_getprop_u32(np, "lcm_params-dsi-customization_esd_check_enable",
			    &lcm_params->dsi.customization_esd_check_enable);
	disp_of_getprop_u32(np, "lcm_params-dsi-esd_check_enable",
			    &lcm_params->dsi.esd_check_enable);
	disp_of_getprop_u32(np, "lcm_params-dsi-lcm_int_te_monitor",
			    &lcm_params->dsi.lcm_int_te_monitor);
	disp_of_getprop_u32(np, "lcm_params-dsi-lcm_int_te_period",
			    &lcm_params->dsi.lcm_int_te_period);
	disp_of_getprop_u32(np, "lcm_params-dsi-lcm_ext_te_monitor",
			    &lcm_params->dsi.lcm_ext_te_monitor);
	disp_of_getprop_u32(np, "lcm_params-dsi-lcm_ext_te_enable",
			    &lcm_params->dsi.lcm_ext_te_enable);
	disp_of_getprop_u32(np, "lcm_params-dsi-noncont_clock", &lcm_params->dsi.noncont_clock);
	disp_of_getprop_u32(np, "lcm_params-dsi-noncont_clock_period",
			    &lcm_params->dsi.noncont_clock_period);
	disp_of_getprop_u32(np, "lcm_params-dsi-clk_lp_per_line_enable",
			    &lcm_params->dsi.clk_lp_per_line_enable);

	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table0-cmd",
			   &lcm_params->dsi.lcm_esd_check_table[0].cmd, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table0-count",
			   &lcm_params->dsi.lcm_esd_check_table[0].count, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table0-para_list",
			   &lcm_params->dsi.lcm_esd_check_table[0].para_list[0], 2);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table1-cmd",
			   &lcm_params->dsi.lcm_esd_check_table[1].cmd, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table1-count",
			   &lcm_params->dsi.lcm_esd_check_table[1].count, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table1-para_list",
			   &lcm_params->dsi.lcm_esd_check_table[1].para_list[0], 2);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table2-cmd",
			   &lcm_params->dsi.lcm_esd_check_table[2].cmd, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table2-count",
			   &lcm_params->dsi.lcm_esd_check_table[2].count, 1);
	disp_of_getprop_u8(np, "lcm_params-dsi-lcm_esd_check_table2-para_list",
			   &lcm_params->dsi.lcm_esd_check_table[2].para_list[0], 2);

	disp_of_getprop_u32(np, "lcm_params-dsi-switch_mode_enable",
			    &lcm_params->dsi.switch_mode_enable);
	disp_of_getprop_u32(np, "lcm_params-dsi-dual_dsi_type", &lcm_params->dsi.dual_dsi_type);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap_en", &lcm_params->dsi.lane_swap_en);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap00", &lcm_params->dsi.lane_swap[0][0]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap01", &lcm_params->dsi.lane_swap[0][1]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap02", &lcm_params->dsi.lane_swap[0][2]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap03", &lcm_params->dsi.lane_swap[0][3]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap04", &lcm_params->dsi.lane_swap[0][4]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap05", &lcm_params->dsi.lane_swap[0][5]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap10", &lcm_params->dsi.lane_swap[1][0]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap11", &lcm_params->dsi.lane_swap[1][1]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap12", &lcm_params->dsi.lane_swap[1][2]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap13", &lcm_params->dsi.lane_swap[1][3]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap14", &lcm_params->dsi.lane_swap[1][4]);
	disp_of_getprop_u32(np, "lcm_params-dsi-lane_swap15", &lcm_params->dsi.lane_swap[1][5]);
	disp_of_getprop_u32(np, "lcm_params-dsi-vertical_vfp_lp", &lcm_params->dsi.vertical_vfp_lp);
	disp_of_getprop_u32(np, "lcm_params-physical_width", &lcm_params->physical_width);
	disp_of_getprop_u32(np, "lcm_params-physical_height", &lcm_params->physical_height);
	disp_of_getprop_u32(np, "lcm_params-od_table_size", &lcm_params->od_table_size);
	disp_of_getprop_u32(np, "lcm_params-od_table", (u32 *)&lcm_params->od_table);
}

void load_lcm_resources_from_DT(disp_lcm_handle *plcm)
{
	/*
	 * Step 1: Get driver ID and panel ID
	 * Step 2: Get the data from DTB by using IDs
	 * Step 3: Set the data back to the LCM driver
	 */
	LCM_DRIVER *lcm_drv = plcm->drv;
	unsigned int driver_id = 0, module_id = 0;
	char comp_driver_id[128] = { 0 };
	char comp_module_id[128] = { 0 };
	struct device_node *np = NULL;
	unsigned int dt_init_cmd_size = 0;
	unsigned char *dt_init_cmd = NULL;
	LCM_PARAMS *dt_lcm_param = NULL;

	struct LCM_setting_table *init_cmd = NULL;
	unsigned int init_cmd_size = 0;
	int i = 0, j = 0, curr = 0;

	/*  Get driver ID and panel ID from DT passed from LK */
	driver_id = lcm_driver_id;
	module_id = lcm_module_id;

	sprintf(comp_driver_id, "mediatek,lcm-%x", driver_id);
	pr_debug("driver_id DT compatible: %s\n", comp_driver_id);
	/*
	 * TODO: Should change to use module id to get DT resources
	 * once actually get the module_id.
	 */
	sprintf(comp_module_id, "mediatek,lcm_params-%x", driver_id);
	pr_debug("module_id DT compatible: %s\n", comp_module_id);

	/* Load LCM data from DT */
	np = of_find_compatible_node(NULL, NULL, comp_driver_id);
	if (!np) {
		pr_debug("LCM Dirver ID DT node: Not fround\n");
	} else {
		const void *data = NULL;
		int data_len = 0;

		data = of_get_property(np, "initialization_command", &data_len);
		if (!data)
			pr_debug("LCM init cmd  DT prop: Not found\n");
		if (data_len > 0) {
			dt_init_cmd = kcalloc(data_len, sizeof(*dt_init_cmd), GFP_KERNEL);
			if (!dt_init_cmd) {
				pr_err("%s:%d, ERROR: Allocation failed\n", __FILE__, __LINE__);
			} else {
				memcpy(dt_init_cmd, data, data_len);
				dt_init_cmd_size = data_len;
			}
		}
	}

	np = of_find_compatible_node(NULL, NULL, comp_module_id);
	if (!np) {
		pr_debug("LCM Params DT node: Not found\n");
	} else {
		dt_lcm_param = kzalloc(sizeof(*dt_lcm_param), GFP_KERNEL);
		if (!dt_lcm_param)
			pr_err("%s:%d, ERROR: Allocation failed\n", __FILE__, __LINE__);

		parse_lcm_params_dt_node(np, dt_lcm_param);
	}

	/* Update the data back to LCM driver */
	/* Convert dt_init_cmd to LCM_setting_table */
	if (dt_init_cmd) {
		init_cmd = kcalloc(MAX_INIT_CNT, sizeof(struct LCM_setting_table), GFP_KERNEL);
		if (!init_cmd)
			pr_err("%s:%d, ERROR: Allocation failed\n", __FILE__, __LINE__);
		memset(init_cmd, 0, MAX_INIT_CNT * sizeof(struct LCM_setting_table));

		while (curr < dt_init_cmd_size) {
			init_cmd[i].cmd = dt_init_cmd[curr++];
			init_cmd[i].count = dt_init_cmd[curr++];
			if (REGFLAG_DELAY != init_cmd[i].cmd)
				for (j = 0; j < init_cmd[i].count; j++)
					init_cmd[i].para_list[j] = dt_init_cmd[curr++];
			i++;
		}
		if (i > MAX_INIT_CNT)
			pr_err("%s:%d, ERROR: Access out of bounds\n", __FILE__, __LINE__);
		init_cmd_size = i;
	}

	if (lcm_drv->set_params)
		lcm_drv->set_params(init_cmd, init_cmd_size, dt_lcm_param);
	else
		pr_debug("LCM set_params not implemented!!!\n");
}
#endif

disp_lcm_handle *disp_lcm_probe(char *plcm_name, LCM_INTERFACE_ID lcm_id)
{
	int lcmindex = 0;
	bool isLCMFound = false;
	bool isLCMInited = false;
	LCM_DRIVER *lcm_drv = NULL;
	LCM_PARAMS *lcm_param = NULL;
	disp_lcm_handle *plcm = NULL;

	DISPPRINT("%s\n", __func__);
	DISPCHECK("plcm_name=%s\n", plcm_name);
	if (_lcm_count() == 0) {
		DISPERR("no lcm driver defined in linux kernel driver\n");
		return NULL;
	} else if (_lcm_count() == 1) {
		if (plcm_name == NULL) {
			lcm_drv = lcm_driver_list[0];

			isLCMFound = true;
			isLCMInited = false;
		} else {
			lcm_drv = lcm_driver_list[0];
			if (strcmp(lcm_drv->name, plcm_name)) {
				DISPERR
				    ("FATAL ERROR!!!LCM Driver defined in kernel is different with LK\n");
				return NULL;
			}

			isLCMInited = true;
			isLCMFound = true;
		}
		lcmindex = 0;
	} else {
		if (plcm_name == NULL) {
			/* TODO: we need to detect all the lcm driver */
		} else {
			int i = 0;

			for (i = 0; i < _lcm_count(); i++) {
				lcm_drv = lcm_driver_list[i];
				if (!strcmp(lcm_drv->name, plcm_name)) {
					isLCMFound = true;
					isLCMInited = true;
					lcmindex = i;
					break;
				}
			}

			DISPERR("FATAL ERROR: can't found lcm driver:%s in linux kernel driver\n",
				plcm_name);
		}
		/* TODO: */
	}

	if (isLCMFound == false) {
		DISPERR("FATAL ERROR!!!No LCM Driver defined\n");
		return NULL;
	}

	plcm = kzalloc(sizeof(uint8_t *) * sizeof(disp_lcm_handle), GFP_KERNEL);
	lcm_param = kzalloc(sizeof(uint8_t *) * sizeof(LCM_PARAMS), GFP_KERNEL);
	if (plcm && lcm_param) {
		plcm->params = lcm_param;
		plcm->drv = lcm_drv;
		plcm->is_inited = isLCMInited;
		plcm->index = lcmindex;
	} else {
		DISPERR("FATAL ERROR!!!kzalloc plcm and plcm->params failed\n");
		goto FAIL;
	}

#if !defined(CONFIG_MTK_LEGACY)
	load_lcm_resources_from_DT(plcm);
#endif

	{
		plcm->drv->get_params(plcm->params);
		plcm->lcm_if_id = plcm->params->lcm_if;

		/* below code is for lcm driver forward compatible */
		if (plcm->params->type == LCM_TYPE_DSI
		    && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED)
			plcm->lcm_if_id = LCM_INTERFACE_DSI0;
		if (plcm->params->type == LCM_TYPE_DPI
		    && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED)
			plcm->lcm_if_id = LCM_INTERFACE_DPI0;
		if (plcm->params->type == LCM_TYPE_DBI
		    && plcm->params->lcm_if == LCM_INTERFACE_NOTDEFINED)
			plcm->lcm_if_id = LCM_INTERFACE_DBI0;

		if ((lcm_id == LCM_INTERFACE_NOTDEFINED) || lcm_id == plcm->lcm_if_id) {
			plcm->lcm_original_width = plcm->params->width;
			plcm->lcm_original_height = plcm->params->height;
			_dump_lcm_info(plcm);
			return plcm;
		}

		DISPERR("the specific LCM Interface [%d] didn't define any lcm driver\n", lcm_id);
		goto FAIL;
	}

FAIL:

	kfree(plcm);
	kfree(lcm_param);
	return NULL;
}


int disp_lcm_init(disp_lcm_handle *plcm, int force)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPPRINT("%s\n", __func__);
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;

		if (lcm_drv->init_power) {
			if (!disp_lcm_is_inited(plcm) || force)
				lcm_drv->init_power();
		}

		if (lcm_drv->init) {
			if (!disp_lcm_is_inited(plcm) || force)
				lcm_drv->init();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->init is null\n");
			return -1;
		}

		return 0;
	}

	DISPERR("plcm is null\n");
	return -1;
}

LCM_PARAMS *disp_lcm_get_params(disp_lcm_handle *plcm)
{
	/* DISPFUNC(); */

	if (_is_lcm_inited(plcm))
		return plcm->params;
	else
		return NULL;
}

LCM_INTERFACE_ID disp_lcm_get_interface_id(disp_lcm_handle *plcm)
{
	DISPFUNC();

	if (_is_lcm_inited(plcm))
		return plcm->lcm_if_id;
	else
		return LCM_INTERFACE_NOTDEFINED;
}

int disp_lcm_update(disp_lcm_handle *plcm, int x, int y, int w, int h, int force)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPDBGFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->update) {
			lcm_drv->update(x, y, w, h);
		} else {
			if (disp_lcm_is_video_mode(plcm))
				; /* do nothing */
			else
				DISPERR("FATAL ERROR, lcm is cmd mode lcm_drv->update is null\n");

			return -1;
		}

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}

/* return 1: esd check fail */
/* return 0: esd check pass */
int disp_lcm_esd_check(disp_lcm_handle *plcm)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->esd_check)
			return lcm_drv->esd_check();

		DISPERR("FATAL ERROR, lcm_drv->esd_check is null\n");
		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return 0;
}



int disp_lcm_esd_recover(disp_lcm_handle *plcm)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->esd_recover) {
			lcm_drv->esd_recover();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->esd_check is null\n");
			return -1;
		}

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}




int disp_lcm_suspend(disp_lcm_handle *plcm)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->suspend) {
			lcm_drv->suspend();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->suspend is null\n");
			return -1;
		}

		if (lcm_drv->suspend_power)
			lcm_drv->suspend_power();

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}




int disp_lcm_resume(disp_lcm_handle *plcm)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;

		if (lcm_drv->resume_power)
			lcm_drv->resume_power();

		if (lcm_drv->resume) {
			lcm_drv->resume();
		} else {
			DISPERR("FATAL ERROR, lcm_drv->resume is null\n");
			return -1;
		}

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}



#ifdef MT_TODO
#error "maybe CABC can be moved into lcm_ioctl??"
#endif
int disp_lcm_set_backlight(disp_lcm_handle *plcm, int level)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->set_backlight) {
			lcm_drv->set_backlight(level);
		} else {
			DISPERR("FATAL ERROR, lcm_drv->set_backlight is null\n");
			return -1;
		}

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}




int disp_lcm_ioctl(disp_lcm_handle *plcm, LCM_IOCTL ioctl, unsigned int arg)
{
	return 0;
}

int disp_lcm_is_inited(disp_lcm_handle *plcm)
{
	if (_is_lcm_inited(plcm))
		return plcm->is_inited;
	else
		return 0;
}

unsigned int disp_lcm_ATA(disp_lcm_handle *plcm)
{
	unsigned int ret = 0;
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->ata_check) {
			ret = lcm_drv->ata_check(NULL);
		} else {
			DISPERR("FATAL ERROR, lcm_drv->ata_check is null\n");
			return 0;
		}

		return ret;
	}

	DISPERR("lcm_drv is null\n");
	return 0;
}

void *disp_lcm_switch_mode(disp_lcm_handle *plcm, int mode)
{
	LCM_DRIVER *lcm_drv = NULL;
	LCM_DSI_MODE_SWITCH_CMD *lcm_cmd = NULL;

	if (_is_lcm_inited(plcm)) {
		if (plcm->params->dsi.switch_mode_enable == 0) {
			DISPERR(" ERROR, Not enable switch in lcm_get_params function\n");
			return NULL;
		}
		lcm_drv = plcm->drv;
		if (lcm_drv->switch_mode) {
			lcm_cmd = (LCM_DSI_MODE_SWITCH_CMD *) lcm_drv->switch_mode(mode);
			lcm_cmd->cmd_if = (unsigned int)(plcm->params->lcm_cmd_if);
		} else {
			DISPERR("FATAL ERROR, lcm_drv->switch_mode is null\n");
			return NULL;
		}

		return (void *)(lcm_cmd);
	}

	DISPERR("lcm_drv is null\n");
	return NULL;
}

int disp_lcm_is_video_mode(disp_lcm_handle *plcm)
{
	/* DISPFUNC(); */
	LCM_PARAMS *lcm_param = NULL;

	if (_is_lcm_inited(plcm))
		lcm_param = plcm->params;
	else
		BUG();

	switch (lcm_param->type) {
	case LCM_TYPE_DBI:
		return false;
	case LCM_TYPE_DSI:
		break;
	case LCM_TYPE_DPI:
		return true;
	default:
		DISPMSG("[LCM] TYPE: unknown\n");
		break;
	}

	if (lcm_param->type == LCM_TYPE_DSI) {
		switch (lcm_param->dsi.mode) {
		case CMD_MODE:
			return false;
		case SYNC_PULSE_VDO_MODE:
		case SYNC_EVENT_VDO_MODE:
		case BURST_VDO_MODE:
			return true;
		default:
			DISPMSG("[LCM] DSI Mode: Unknown\n");
			break;
		}
	}

	BUG();
	return 0;
}

int disp_lcm_set_cmd(disp_lcm_handle *plcm, void *handle, int *lcm_cmd, unsigned int cmd_num)
{
	LCM_DRIVER *lcm_drv = NULL;

	DISPFUNC();
	if (_is_lcm_inited(plcm)) {
		lcm_drv = plcm->drv;
		if (lcm_drv->set_cmd) {
			lcm_drv->set_cmd(handle, lcm_cmd, cmd_num);
		} else {
			DISPERR("FATAL ERROR, lcm_drv->set_cmd is null\n");
			return -1;
		}

		return 0;
	}

	DISPERR("lcm_drv is null\n");
	return -1;
}
