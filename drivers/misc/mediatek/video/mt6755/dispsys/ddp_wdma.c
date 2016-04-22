#define LOG_TAG "WDMA"
#include "ddp_log.h"
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
#include <linux/delay.h>
#include "ddp_reg.h"
#include "ddp_matrix_para.h"
#include "ddp_info.h"
#include "ddp_wdma.h"
#include "primary_display.h"
#include "m4u.h"

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

static char *wdma_get_status(unsigned int status)
{
	switch (status) {
	case 0x1:
		return "idle";
	case 0x2:
		return "clear";
	case 0x4:
		return "prepare";
	case 0x8:
		return "prepare";
	case 0x10:
		return "data_running";
	case 0x20:
		return "eof_wait";
	case 0x40:
		return "soft_reset_wait";
	case 0x80:
		return "eof_done";
	case 0x100:
		return "soft_reset_done";
	case 0x200:
		return "frame_complete";
	}
	return "unknown";

}

static unsigned int wdma_index(DISP_MODULE_ENUM module)
{
	int idx = 0;

	switch (module) {
	case DISP_MODULE_WDMA0:
		idx = 0;
		break;
	case DISP_MODULE_WDMA1:
		idx = 1;
		break;
	default:
		DDPERR("[DDP] error: invalid wdma module=%d\n", module);	/* invalid module */
		ASSERT(0);
	}
	return idx;
}

static int wdma_start(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTEN, 0x03);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_EN, 0x01);

	return 0;
}

static int wdma_stop(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTEN, 0x00);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_EN, 0x00);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTSTA, 0x00);

	return 0;
}

static int wdma_reset(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int delay_cnt = 0;
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_RST, 0x01);	/* trigger soft reset */
	if (!handle) {
		while ((DISP_REG_GET(idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_FLOW_CTRL_DBG) &
			0x1) == 0) {
			delay_cnt++;
			udelay(10);
			if (delay_cnt > 2000) {
				DDPERR("wdma%d reset timeout!\n", idx);
				break;
			}
		}
	} else {
		/* add comdq polling */
	}
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_RST, 0x0);	/* trigger soft reset */

	return 0;
}

static int wdma_config_yuv420(DISP_MODULE_ENUM module,
			      enum UNIFIED_COLOR_FMT fmt,
			      unsigned int dstPitch,
			      unsigned int Height,
			      unsigned long dstAddress, DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;
	unsigned int u_off = 0;
	unsigned int v_off = 0;
	unsigned int u_stride = 0;
	unsigned int y_size = 0;
	unsigned int u_size = 0;
	unsigned int stride = dstPitch;
	int has_v = 1;

	if (fmt != UFMT_YV12 && fmt != UFMT_I420 && fmt != UFMT_NV12 && fmt != UFMT_NV21)
		return 0;

	if (fmt == UFMT_YV12) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		u_off = y_size;
		v_off = y_size + u_size;
	} else if (fmt == UFMT_I420) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		v_off = y_size;
		u_off = y_size + u_size;
	} else if (fmt == UFMT_NV12 || fmt == UFMT_NV21) {
		y_size = stride * Height;
		u_stride = stride / 2;
		u_size = u_stride * Height / 2;
		u_off = y_size;
		has_v = 0;
	}

	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR1, dstAddress + u_off);
		if (has_v)
			DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR2,
				     dstAddress + v_off);
	} else {
		int m4u_port;

		m4u_port = idx == 0 ? M4U_PORT_DISP_WDMA0 : M4U_PORT_DISP_WDMA1;

		cmdqRecWriteSecure(handle, disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR1),
				   CMDQ_SAM_H_2_MVA, dstAddress, u_off, u_size, m4u_port);
		if (has_v)
			cmdqRecWriteSecure(handle,
					   disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR2),
					   CMDQ_SAM_H_2_MVA, dstAddress, v_off, u_size, m4u_port);
	}
	DISP_REG_SET_FIELD(handle, DST_W_IN_BYTE_FLD_DST_W_IN_BYTE,
			   idx_offst + DISP_REG_WDMA_DST_UV_PITCH, u_stride);
	return 0;
}

static int wdma_config(DISP_MODULE_ENUM module,
		       unsigned srcWidth,
		       unsigned srcHeight,
		       unsigned clipX,
		       unsigned clipY,
		       unsigned clipWidth,
		       unsigned clipHeight,
		       enum UNIFIED_COLOR_FMT out_format,
		       unsigned long dstAddress,
		       unsigned dstPitch,
		       unsigned int useSpecifiedAlpha,
		       unsigned char alpha, DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int output_swap = ufmt_get_byteswap(out_format);
	unsigned int is_rgb = ufmt_get_rgb(out_format);
	unsigned int out_fmt_reg = ufmt_get_format(out_format);
	int color_matrix = 0x2;	/* 0010 RGB_TO_BT601 */
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;
	size_t size = dstPitch * clipHeight;

	DDPDBG("%s,src(w%d,h%d),clip(x%d,y%d,w%d,h%d),fmt=%s,addr=0x%lx,pitch=%d,s_alfa=%d,alpa=%d,hnd=0x%p,sec%d\n",
	     ddp_get_module_name(module), srcWidth, srcHeight, clipX, clipY, clipWidth, clipHeight,
	     unified_color_fmt_name(out_format), dstAddress, dstPitch, useSpecifiedAlpha, alpha,
	     handle, sec);

	/* should use OVL alpha instead of sw config */
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_SRC_SIZE, srcHeight << 16 | srcWidth);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_CLIP_COORD, clipY << 16 | clipX);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_CLIP_SIZE, clipHeight << 16 | clipWidth);
	DISP_REG_SET_FIELD(handle, CFG_FLD_OUT_FORMAT, idx_offst + DISP_REG_WDMA_CFG, out_fmt_reg);

	if (!is_rgb) {
		/* set DNSP for UYVY and YUV_3P format for better quality */
		wdma_config_yuv420(module, out_format, dstPitch, clipHeight, dstAddress, sec,
				   handle);
		/*user internal matrix */
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, idx_offst + DISP_REG_WDMA_CFG, 1);
		DISP_REG_SET_FIELD(handle, CFG_FLD_INT_MTX_SEL, idx_offst + DISP_REG_WDMA_CFG,
				   color_matrix);
	} else {
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
	}
	DISP_REG_SET_FIELD(handle, CFG_FLD_SWAP, idx_offst + DISP_REG_WDMA_CFG, output_swap);
	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR0, dstAddress);
	} else {
		int m4u_port;

		m4u_port = idx == 0 ? M4U_PORT_DISP_WDMA0 : M4U_PORT_DISP_WDMA1;

		/* for sec layer, addr variable stores sec handle */
		/* we need to pass this handle and offset to cmdq driver */
		/* cmdq sec driver will help to convert handle to correct address */
		cmdqRecWriteSecure(handle, disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR0),
				   CMDQ_SAM_H_2_MVA, dstAddress, 0, size, m4u_port);
	}
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_W_IN_BYTE, dstPitch);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_SEL, idx_offst + DISP_REG_WDMA_ALPHA,
			   useSpecifiedAlpha);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_VALUE, idx_offst + DISP_REG_WDMA_ALPHA, alpha);

	return 0;
}

static int wdma_clock_on(DISP_MODULE_ENUM module, void *handle)
{
	/* DDPMSG("wmda%d_clock_on\n",idx); */
	/* do not set CG */
/*
#ifdef ENABLE_CLK_MGR
#ifdef CONFIG_MTK_CLKMGR
	if (idx == 0)
		enable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
	else
		enable_clock(MT_CG_DISP0_DISP_WDMA1, "WDMA1");
#else
	if (idx == 0)
		ddp_clk_enable(DISP0_DISP_WDMA0);
	else
		ddp_clk_enable(DISP0_DISP_WDMA1);
#endif
#endif
*/
	/* DCM Setting -- Enable DCM */
	DISP_REG_MASK(NULL, DISP_REG_WDMA_EN, 0x80000000, 0x80000000);
	return 0;
}

static int wdma_clock_off(DISP_MODULE_ENUM module, void *handle)
{
	/* DDPMSG("wdma%d_clock_off\n",idx); */
	/* do not set CG */
/*
#ifdef ENABLE_CLK_MGR
#ifdef CONFIG_MTK_CLKMGR
	if (idx == 0)
		disable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
	else
		disable_clock(MT_CG_DISP0_DISP_WDMA1, "WDMA1");
#else
	if (idx == 0)
		ddp_clk_disable(DISP0_DISP_WDMA0);
	else
		ddp_clk_disable(DISP0_DISP_WDMA1);
#endif

#endif
*/
	return 0;
}

void wdma_dump_analysis(DISP_MODULE_ENUM module)
{
	unsigned int index = wdma_index(module);
	unsigned int idx_offst = index * DISP_WDMA_INDEX_OFFSET;

	DDPDUMP("==DISP WDMA%d ANALYSIS==\n", index);
	DDPDUMP("wdma%d:en=%d,w=%d,h=%d,clip=(%d,%d,%d,%d),pitch=(W=%d,UV=%d),addr=(0x%x,0x%x,0x%x),fmt=%s\n",
	     index, DISP_REG_GET(DISP_REG_WDMA_EN + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + idx_offst),
	     unified_color_fmt_name(display_fmt_reg_to_unified_fmt
				    ((DISP_REG_GET(DISP_REG_WDMA_CFG + idx_offst) >> 4) & 0xf,
				     (DISP_REG_GET(DISP_REG_WDMA_CFG + idx_offst) >> 11) & 0x1, 0))
	    );
	DDPDUMP("wdma%d:status=%s,in_req=%d(prev sent data),in_ack=%d(ask data to prev),exec=%d,in_pix=(L:%d,P:%d)\n",
		index,
		wdma_get_status(DISP_REG_GET_FIELD
				(FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL,
				 DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst)),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_REQ,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_ACK,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + idx_offst) & 0x1f,
		(DISP_REG_GET(DISP_REG_WDMA_CT_DBG + idx_offst) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + idx_offst) & 0xffff);

}

void wdma_dump_reg(DISP_MODULE_ENUM module)
{
	unsigned int idx = wdma_index(module);
	unsigned int off_sft = idx * DISP_WDMA_INDEX_OFFSET;

	DDPDUMP("== START: DISP WDMA%d REGS ==\n", idx);

	DDPDUMP("WDMA:0x000=0x%08x,0x004=0x%08x,0x008=0x%08x,0x00c=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_INTEN + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_INTSTA + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_EN + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_RST + off_sft));

	DDPDUMP("WDMA:0x010=0x%08x,0x014=0x%08x,0x018=0x%08x,0x01c=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_SMI_CON + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CFG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + off_sft));

	DDPDUMP("WDMA:0x020=0x%08x,0x028=0x%08x,0x02c=0x%08x,0x038=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_ALPHA + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_BUF_CON1 + off_sft));

	DDPDUMP("WDMA:0x03c=0x%08x,0x058=0x%08x,0x05c=0x%08x,0x060=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_BUF_CON2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD0 + off_sft));

	DDPDUMP("WDMA:0x064=0x%08x,0x078=0x%08x,0x080=0x%08x,0x084=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET1 + off_sft));

	DDPDUMP("WDMA:0x088=0x%08x,0x0a0=0x%08x,0x0a4=0x%08x,0x0a8=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_FLOW_CTRL_DBG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + off_sft));

	DDPDUMP("WDMA:0x0ac=0x%08x,0xf00=0x%08x,0xf04=0x%08x,0xf08=0x%08x,\n",
		DISP_REG_GET(DISP_REG_WDMA_DEBUG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + off_sft));

#ifdef __ENABLE_WDMA_PROC_TRACK__
	DDPDUMP("WDMA:0x090=0x%08x,0x094=0x%08x,0x098=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_PROC_TRACK_CON_0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PROC_TRACK_CON_1 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PROC_TRACK_CON_2 + off_sft));

	void *emi_va = ioremap_nocache(0x10203000, 0x1000);

	DDPDUMP("0x10203018 = 0x%08x\n", *(unsigned int *)(emi_va+0x18));
	iounmap(emi_va);
#endif
	DDPDUMP("-- END: DISP WDMA%d REGS --\n", idx);
	return;
}

static int wdma_dump(DISP_MODULE_ENUM module, int level)
{
	wdma_dump_analysis(module);
	wdma_dump_reg(module);

	return 0;
}

static int wdma_golden_setting(DISP_MODULE_ENUM module, enum dst_module_type dst_mod_type,
	unsigned int width, unsigned int height, void *cmdq)
{
	unsigned int regval;
	int wdma_idx = wdma_index(module);
	unsigned long idx_offset = wdma_idx * DISP_WDMA_INDEX_OFFSET;
	unsigned long res = width * height;
	unsigned long res_HD = 720 * 1280;
	/* DISP_REG_WDMA_BUF_CON1 */
	regval = 0;
	if (dst_mod_type == DST_MOD_REAL_TIME) {
		regval |= REG_FLD_VAL(BUF_CON1_FLD_ULTRA_ENABLE, 1);
		regval |= REG_FLD_VAL(BUF_CON1_FLD_PRE_ULTRA_ENABLE, 1);
	} else {
		regval |= REG_FLD_VAL(BUF_CON1_FLD_ULTRA_ENABLE, 0);
		regval |= REG_FLD_VAL(BUF_CON1_FLD_PRE_ULTRA_ENABLE, 0);
	}

	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(BUF_CON1_FLD_FRAME_END_ULTRA, 1);
	else
		regval |= REG_FLD_VAL(BUF_CON1_FLD_FRAME_END_ULTRA, 0);

	regval |= REG_FLD_VAL(BUF_CON1_FLD_ISSUE_REQ_TH, 64);
	regval |= REG_FLD_VAL(BUF_CON1_FLD_FIFO_PSEUDO_SIZE, 256);

	DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_BUF_CON1, regval);

	/* DISP_REG_WDMA_BUF_CON2 */
	regval = 0;
	if (res > res_HD)
		regval |= REG_FLD_VAL(BUF_CON2_FLD_ULTRA_TH_HIGH_OFS, 45);
	else
		regval |= REG_FLD_VAL(BUF_CON2_FLD_ULTRA_TH_HIGH_OFS, 20);

	regval |= REG_FLD_VAL(BUF_CON2_FLD_PRE_ULTRA_TH_HIGH_OFS, 1);

	if (res > res_HD)
		regval |= REG_FLD_VAL(BUF_CON2_FLD_ULTRA_TH_LOW_OFS, 24);
	else
		regval |= REG_FLD_VAL(BUF_CON2_FLD_ULTRA_TH_LOW_OFS, 10);

	if (res > res_HD)
		regval |= REG_FLD_VAL(BUF_CON2_FLD_PRE_ULTRA_TH_LOW, 93);
	else
		regval |= REG_FLD_VAL(BUF_CON2_FLD_PRE_ULTRA_TH_LOW, 184);

	DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_BUF_CON2, regval);

#ifdef __ENABLE_WDMA_PROC_TRACK__
	/* WDMA proc track */
	regval = 0;
	if (dst_mod_type == DST_MOD_REAL_TIME) {
		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_0, 0);
		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_1, 0);
		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_2, 0);
		cmdqRecWrite(cmdq, 0x10203018, 0, 0xffff0000);
	} else {
		regval |= REG_FLD_VAL(FLD_WDMA_PROC_TRACK_CON0_IGNORE_INIT_LEGECY, 1);
		regval |= REG_FLD_VAL(FLD_WDMA_PROC_TRACK_CON0_STOP_GREQ_EN, 1);
		regval |= REG_FLD_VAL(FLD_WDMA_PROC_TRACK_CON0_PREULTRA_EN, 1);
		regval |= REG_FLD_VAL(FLD_WDMA_PROC_TRACK_CON0_TRACK_WND, 256);

		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_0, regval);
		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_1, 0x8000);
		DISP_REG_SET(cmdq, idx_offset + DISP_REG_WDMA_PROC_TRACK_CON_2, 3*0x8000);
		cmdqRecWrite(cmdq, 0x10203018, 0xffff0000, 0xffff0000);
	}
#endif
	return 0;
}


static int wdma_check_input_param(WDMA_CONFIG_STRUCT *config)
{
	if (!is_unified_color_fmt_supported(config->outputFormat)) {
		DDPERR("wdma parameter invalidate outfmt %s:0x%x\n",
		       unified_color_fmt_name(config->outputFormat), config->outputFormat);
		return -1;
	}

	if (config->dstAddress == 0 || config->srcWidth == 0 || config->srcHeight == 0) {
		DDPERR("wdma parameter invalidate, addr=0x%lx, w=%d, h=%d\n",
		       config->dstAddress, config->srcWidth, config->srcHeight);
		return -1;
	}
	return 0;
}

static int wdma_is_sec[2];

static int wdma_config_l(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *handle)
{

	WDMA_CONFIG_STRUCT *config = &pConfig->wdma_config;
	int wdma_idx = wdma_index(module);
	CMDQ_ENG_ENUM cmdq_engine;
	CMDQ_EVENT_ENUM cmdq_event;
	/*CMDQ_EVENT_ENUM cmdq_event_nonsec_end;*/

	if (!pConfig->wdma_dirty)
		return 0;

	cmdq_engine = wdma_idx == 0 ? CMDQ_ENG_DISP_WDMA0 : CMDQ_ENG_DISP_WDMA1;
	cmdq_event  = wdma_idx == 0 ? CMDQ_EVENT_DISP_WDMA0_EOF : CMDQ_EVENT_DISP_WDMA1_EOF;
	/*cmdq_event_nonsec_end  = wdma_idx == 0 ?
	CMDQ_SYNC_DISP_WDMA0_2NONSEC_END : CMDQ_SYNC_DISP_WDMA1_2NONSEC_END;*/

	if (config->security == DISP_SECURE_BUFFER) {
		cmdqRecSetSecure(handle, 1);
		/* set engine as sec */
		cmdqRecSecureEnablePortSecurity(handle, (1LL << cmdq_engine));
		cmdqRecSecureEnableDAPC(handle, (1LL << cmdq_engine));
		if (wdma_is_sec[wdma_idx] == 0)
			DDPMSG("[SVP] switch wdma%d to sec\n", wdma_idx);
		wdma_is_sec[wdma_idx] = 1;
	} else {
		if (wdma_is_sec[wdma_idx]) {
			/* wdma is in sec stat, we need to switch it to nonsec */
			cmdqRecHandle nonsec_switch_handle;
			int ret;

			ret = cmdqRecCreate(CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
					  &(nonsec_switch_handle));
			if (ret)
				DDPAEE("[SVP]fail to create disable handle %s ret=%d\n",
				       __func__, ret);

			cmdqRecReset(nonsec_switch_handle);

			if (wdma_idx == 0) {
				/*Primary Mode*/
				if (primary_display_is_decouple_mode())
					cmdqRecWaitNoClear(nonsec_switch_handle, cmdq_event);
				else
					_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);
			} else {
				/*External Mode*/
				/*ovl1->wdma1*/
				/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
				cmdqRecWaitNoClear(nonsec_switch_handle, CMDQ_SYNC_DISP_EXT_STREAM_EOF);
			}

			/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
			cmdqRecSetSecure(nonsec_switch_handle, 1);

			/*in fact, dapc/port_sec will be disabled by cmdq */
			cmdqRecSecureEnablePortSecurity(nonsec_switch_handle, (1LL << cmdq_engine));
			cmdqRecSecureEnableDAPC(nonsec_switch_handle, (1LL << cmdq_engine));
			/*cmdqRecSetEventToken(nonsec_switch_handle, cmdq_event_nonsec_end);*/
			/*cmdqRecFlushAsync(nonsec_switch_handle);*/
			cmdqRecFlush(nonsec_switch_handle);
			cmdqRecDestroy(nonsec_switch_handle);
			/*cmdqRecWait(handle, cmdq_event_nonsec_end);*/
			DDPMSG("[SVP] switch wdma%d to nonsec\n", wdma_idx);
		}
		wdma_is_sec[wdma_idx] = 0;
	}

	if (wdma_check_input_param(config) == 0) {
		enum dst_module_type dst_mod_type;

		wdma_config(module,
			    config->srcWidth,
			    config->srcHeight,
			    config->clipX,
			    config->clipY,
			    config->clipWidth,
			    config->clipHeight,
			    config->outputFormat,
			    config->dstAddress,
			    config->dstPitch,
			    config->useSpecifiedAlpha, config->alpha, config->security, handle);

		dst_mod_type = dpmgr_path_get_dst_module_type(pConfig->path_handle);
		wdma_golden_setting(module, dst_mod_type, config->srcWidth, config->srcHeight, handle);
	}
	return 0;
}

unsigned int ddp_wdma_get_cur_addr(void)
{
	return INREG32(DISP_REG_WDMA_DST_ADDR0);
}

DDP_MODULE_DRIVER ddp_driver_wdma = {
	.module = DISP_MODULE_WDMA0,
	.init = wdma_clock_on,
	.deinit = wdma_clock_off,
	.config = wdma_config_l,
	.start = wdma_start,
	.trigger = NULL,
	.stop = wdma_stop,
	.reset = wdma_reset,
	.power_on = wdma_clock_on,
	.power_off = wdma_clock_off,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = wdma_dump,
	.bypass = NULL,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
};
