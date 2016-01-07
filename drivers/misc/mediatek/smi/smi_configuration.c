#include <asm/io.h>
#include <linux/string.h>
#include "mt_smi.h"
#include "smi_configuration.h"
#include "smi_common.h"
#include "smi_reg.h"

/* add static after all platform setting parameters moved to here */
int is_default_value_saved;
unsigned int default_val_smi_l1arb[SMI_LARB_NR] = { 0 };

#define SMI_LARB_NUM_MAX 8

#if defined(SMI_D1)
unsigned int smi_dbg_disp_mask = 1;
unsigned int smi_dbg_vdec_mask = 2;
unsigned int smi_dbg_imgsys_mask = 4;
unsigned int smi_dbg_venc_mask = 8;
unsigned int smi_dbg_mjc_mask = 0;

unsigned long smi_common_l1arb_offset[SMI_LARB_NR] = {
	REG_OFFSET_SMI_L1ARB0, REG_OFFSET_SMI_L1ARB1, REG_OFFSET_SMI_L1ARB2
};

unsigned long smi_larb0_debug_offset[SMI_LARB0_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb1_debug_offset[SMI_LARB1_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb2_debug_offset[SMI_LARB2_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb3_debug_offset[SMI_LARB3_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM] = {
	0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x220, 0x230, 0x234, 0x238, 0x400, 0x404, 0x408,
	0x40C, 0x430, 0x440
};

int smi_larb_debug_offset_num[SMI_LARB_NR] = {
	SMI_LARB0_DEBUG_OFFSET_NUM, SMI_LARB1_DEBUG_OFFSET_NUM, SMI_LARB2_DEBUG_OFFSET_NUM,
	SMI_LARB3_DEBUG_OFFSET_NUM
};

unsigned long *smi_larb_debug_offset[SMI_LARB_NR] = {
	smi_larb0_debug_offset, smi_larb1_debug_offset, smi_larb2_debug_offset,
	smi_larb3_debug_offset
};

#define SMI_PROFILE_SETTING_COMMON_INIT_NUM 7
#define SMI_VC_SETTING_NUM SMI_LARB_NR


/* vc setting */
struct SMI_SETTING_VALUE smi_vc_setting[SMI_VC_SETTING_NUM] = {
	{0x20, 0}, {0x20, 2}, {0x20, 1}, {0x20, 1}
};

/* init_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_init[SMI_PROFILE_SETTING_COMMON_INIT_NUM] = {
	{0, 0}, {0, 0x1000}, {0, 0x1000}, {0, 0x1000},
	{0x100, 0x1b},
	{0x234, (0x1 << 31) + (0x1d << 26) + (0x1f << 21) + (0x0 << 20) + (0x3 << 15)
	 + (0x4 << 10) + (0x4 << 5) + 0x5},
	{0x230, 0x1f + (0x8 << 4) + (0x7 << 9)}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_init[SMI_LARB0_PORT_NUM] = {
	{0x200, 0x1f}, {0x204, 4}, {0x208, 6}, {0x20c, 0x1f}, {0x210, 4}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_init[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_init[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_init[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING init_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

#define SMI_PROFILE_SETTING_COMMON_VR_NUM SMI_LARB_NR
/* vr_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vr[SMI_PROFILE_SETTING_COMMON_VR_NUM] = {
	{0, 0x11F1}, {0, 0x1000}, {0, 0x120A}, {0, 0x11F3}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vr[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 2}, {0x218, 4}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vr[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vr[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 4}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 4}, {0x22c, 1}, {0x230, 2}, {0x234, 2}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vr[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 3}, {0x230, 2}
};

struct SMI_SETTING vr_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

#define SMI_PROFILE_SETTING_COMMON_VP_NUM SMI_LARB_NR
/* vp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vp[SMI_PROFILE_SETTING_COMMON_VP_NUM] = {
	{0, 0x1262}, {0, 0x11E9}, {0, 0x1000}, {0, 0x123D}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vp[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 1}, {0x208, 2}, {0x20c, 1}, {0x210, 3}, {0x214, 1}, {0x218, 4}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vp[SMI_LARB1_PORT_NUM] = {
	{0x200, 0xb}, {0x204, 0xe}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vp[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vp[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 3}, {0x230, 2}
};

struct SMI_SETTING vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

/* vr series */
struct SMI_SETTING icfp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING vr_slow_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING venc_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

/* vp series */
struct SMI_SETTING vpwfd_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

struct SMI_SETTING swdec_vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

/* init series */
struct SMI_SETTING mm_gpu_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

struct SMI_SETTING ui_idle_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi4k_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING vss_setting_config = { 0, NULL, {0}, {0} };

#elif defined(SMI_D3)
unsigned int smi_dbg_disp_mask = 1;
unsigned int smi_dbg_vdec_mask = 2;
unsigned int smi_dbg_imgsys_mask = 4;
unsigned int smi_dbg_venc_mask = 8;
unsigned int smi_dbg_mjc_mask = 0;

unsigned long smi_common_l1arb_offset[SMI_LARB_NR] = {
	REG_OFFSET_SMI_L1ARB0, REG_OFFSET_SMI_L1ARB1, REG_OFFSET_SMI_L1ARB2, REG_OFFSET_SMI_L1ARB3
};

static unsigned long smi_larb0_debug_offset[SMI_LARB0_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

static unsigned long smi_larb1_debug_offset[SMI_LARB1_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

static unsigned long smi_larb2_debug_offset[SMI_LARB2_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

static unsigned long smi_larb3_debug_offset[SMI_LARB3_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM] = {
	0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x220, 0x230, 0x234, 0x238, 0x400, 0x404, 0x408,
	0x40C, 0x430, 0x440
};

int smi_larb_debug_offset_num[SMI_LARB_NR] = {
	SMI_LARB0_DEBUG_OFFSET_NUM, SMI_LARB1_DEBUG_OFFSET_NUM, SMI_LARB2_DEBUG_OFFSET_NUM,
	SMI_LARB3_DEBUG_OFFSET_NUM
};

unsigned long *smi_larb_debug_offset[SMI_LARB_NR] = {
	smi_larb0_debug_offset, smi_larb1_debug_offset, smi_larb2_debug_offset,
	smi_larb3_debug_offset
};


#define SMI_PROFILE_SETTING_COMMON_INIT_NUM 7
#define SMI_VC_SETTING_NUM SMI_LARB_NR


/* vc setting */
struct SMI_SETTING_VALUE smi_vc_setting[SMI_VC_SETTING_NUM] = {
	{0x20, 0}, {0x20, 2}, {0x20, 1}, {0x20, 1}
};

/* init_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_init[SMI_PROFILE_SETTING_COMMON_INIT_NUM] = {
	{0, 0}, {0, 0x1000}, {0, 0x1000}, {0, 0x1000},
	{0x100, 0xb},
	{0x234, (0x1 << 31) + (0x1d << 26) + (0x1f << 21) + (0x0 << 20) + (0x3 << 15)
	 + (0x4 << 10) + (0x4 << 5) + 0x5},
	{0x230, 0xf + (0x8 << 4) + (0x7 << 9)}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_init[SMI_LARB0_PORT_NUM] = {
	{0x200, 0x1f}, {0x204, 8}, {0x208, 6}, {0x20c, 0x1f}, {0x210, 4}, {0x214, 1}, {0x218, 0},
	    {0x21c, 2},
	{0x220, 1}, {0x224, 3}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_init[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_init[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_init[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING init_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

#define SMI_PROFILE_SETTING_COMMON_VR_NUM SMI_LARB_NR
/* vr_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vr[SMI_PROFILE_SETTING_COMMON_VR_NUM] = {
	{0, 0x1417}, {0, 0x1000}, {0, 0x11D0}, {0, 0x11F8}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vr[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xa}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1},
	    {0x21c, 4},
	{0x220, 1}, {0x224, 6}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vr[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vr[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 2}, {0x21c,
											     1},
	{0x220, 2}, {0x224, 1}, {0x228, 1}, {0x22c, 8}, {0x230, 1}, {0x234, 1}, {0x238, 2}, {0x23c,
											     2},
	{0x240, 2}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vr[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 3}, {0x230, 2}
};

struct SMI_SETTING vr_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

#define SMI_PROFILE_SETTING_COMMON_VP_NUM SMI_LARB_NR
/* vp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vp[SMI_PROFILE_SETTING_COMMON_VP_NUM] = {
	{0, 0x1262}, {0, 0x11E9}, {0, 0x1000}, {0, 0x123D}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vp[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 1}, {0x208, 2}, {0x20c, 1}, {0x210, 3}, {0x214, 1}, {0x218, 4}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vp[SMI_LARB1_PORT_NUM] = {
	{0x200, 0xb}, {0x204, 0xe}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vp[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vp[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 3}, {0x230, 2}
};

struct SMI_SETTING vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

#define SMI_PROFILE_SETTING_COMMON_VPWFD_NUM SMI_LARB_NR
/* vpwfd_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vpwfd[SMI_PROFILE_SETTING_COMMON_VPWFD_NUM] = {
	{0, 0x14B6}, {0, 0x11EE}, {0, 0x1000}, {0, 0x11F2}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vpwfd[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xc}, {0x204, 8}, {0x208, 6}, {0x20c, 0xc}, {0x210, 4}, {0x214, 1}, {0x218, 1},
	    {0x21c, 3},
	{0x220, 2}, {0x224, 5}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vpwfd[SMI_LARB1_PORT_NUM] = {
	{0x200, 0xb}, {0x204, 0xe}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vpwfd[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vpwfd[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 3}, {0x230, 2}
};

struct SMI_SETTING vpwfd_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VPWFD_NUM, smi_profile_setting_common_vpwfd,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vpwfd, smi_profile_setting_larb1_vpwfd,
	 smi_profile_setting_larb2_vpwfd, smi_profile_setting_larb3_vpwfd}
};

#define SMI_PROFILE_SETTING_COMMON_ICFP_NUM SMI_LARB_NR
/* icfp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_icfp[SMI_PROFILE_SETTING_COMMON_ICFP_NUM] = {
	{0, 0x14E2}, {0, 0x1000}, {0, 0x1310}, {0, 0x106F}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_icfp[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xe}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1},
	    {0x21c, 2},
	{0x220, 2}, {0x224, 3}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_icfp[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_icfp[SMI_LARB2_PORT_NUM] = {
	{0x200, 0xc}, {0x204, 4}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1},
	    {0x21c, 1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 3}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_icfp[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING icfp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_ICFP_NUM, smi_profile_setting_common_icfp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_icfp, smi_profile_setting_larb1_icfp,
	 smi_profile_setting_larb2_icfp, smi_profile_setting_larb3_icfp}
};

/* vr series */
struct SMI_SETTING vr_slow_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING venc_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

/* vp series */
struct SMI_SETTING swdec_vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

/* init seris */
struct SMI_SETTING mm_gpu_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

struct SMI_SETTING vss_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING ui_idle_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi4k_setting_config = { 0, NULL, {0}, {0} };

#elif defined(SMI_J)
unsigned int smi_dbg_disp_mask = 1;
unsigned int smi_dbg_vdec_mask = 2;
unsigned int smi_dbg_imgsys_mask = 4;
unsigned int smi_dbg_venc_mask = 8;
unsigned int smi_dbg_mjc_mask = 0;

unsigned long smi_common_l1arb_offset[SMI_LARB_NR] = {
	REG_OFFSET_SMI_L1ARB0, REG_OFFSET_SMI_L1ARB1, REG_OFFSET_SMI_L1ARB2, REG_OFFSET_SMI_L1ARB3
};

unsigned long smi_larb0_debug_offset[SMI_LARB0_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb1_debug_offset[SMI_LARB1_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb2_debug_offset[SMI_LARB2_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb3_debug_offset[SMI_LARB3_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM] = {
	0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x220, 0x230, 0x234, 0x238, 0x400, 0x404, 0x408,
	0x40C, 0x430, 0x440
};

int smi_larb_debug_offset_num[SMI_LARB_NR] = {
	SMI_LARB0_DEBUG_OFFSET_NUM, SMI_LARB1_DEBUG_OFFSET_NUM, SMI_LARB2_DEBUG_OFFSET_NUM,
	SMI_LARB3_DEBUG_OFFSET_NUM
};

unsigned long *smi_larb_debug_offset[SMI_LARB_NR] = {
	smi_larb0_debug_offset, smi_larb1_debug_offset, smi_larb2_debug_offset,
	smi_larb3_debug_offset
};

#define SMI_PROFILE_SETTING_COMMON_INIT_NUM 7
#define SMI_VC_SETTING_NUM SMI_LARB_NR
#define SMI_INITSETTING_LARB0_NUM (SMI_LARB0_PORT_NUM + 4)


/* vc setting */
struct SMI_SETTING_VALUE smi_vc_setting[SMI_VC_SETTING_NUM] = {
	{0x20, 0}, {0x20, 2}, {0x20, 1}, {0x20, 1}
};

/* ISP HRT setting */
struct SMI_SETTING_VALUE smi_isp_hrt_setting[SMI_LARB_NR] = {
	{0x24, 0}, {0x24, 0}, {0x24, 0}, {0x24, 0}
};

/* init_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_init[SMI_PROFILE_SETTING_COMMON_INIT_NUM] = {
	{0, 0x15AE}, {0, 0x1000}, {0, 0x1000}, {0, 0x1000},
	{0x100, 0xb},
	{0x234, ((0x1 << 31) + (0x1d << 26) + (0x1f << 21) + (0x0 << 20) + (0x3 << 15)
		 + (0x4 << 10) + (0x4 << 5) + 0x5)},
	{0x230, 0xf + (0x8 << 4) + (0x7 << 9)}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_init[SMI_INITSETTING_LARB0_NUM] = {
	{0x200, 31}, {0x204, 8}, {0x208, 6}, {0x20c, 31}, {0x210, 4}, {0x214, 1}, {0x218, 31},
	    {0x21c, 31},
	{0x220, 2}, {0x224, 1}, {0x228, 3}, {0x100, 5}, {0x10c, 5}, {0x118, 5}, {0x11c, 5}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_init[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_init[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 4}, {0x208, 2}, {0x20c, 2}, {0x210, 2}, {0x214, 1}, {0x218, 2}, {0x21c,
											     2},
	{0x220, 2}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 2}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_init[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING init_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_INITSETTING_LARB0_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

#define SMI_PROFILE_SETTING_COMMON_VR_NUM SMI_LARB_NR
/* vr_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vr[SMI_PROFILE_SETTING_COMMON_VR_NUM] = {
	{0, 0x1393}, {0, 0x1000}, {0, 0x1205}, {0, 0x11D4}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vr[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xe}, {0x204, 8}, {0x208, 4}, {0x20c, 0xe}, {0x210, 4}, {0x214, 1}, {0x218, 0xe},
	    {0x21c, 0xe},
	{0x220, 2}, {0x224, 1}, {0x228, 2}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vr[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vr[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 4}, {0x208, 2}, {0x20c, 2}, {0x210, 2}, {0x214, 1}, {0x218, 2}, {0x21c,
											     2},
	{0x220, 2}, {0x224, 1}, {0x228, 1}, {0x22c, 2}, {0x230, 1}, {0x234, 2}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vr[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 1}, {0x230, 4}
};

struct SMI_SETTING vr_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

#define SMI_PROFILE_SETTING_COMMON_VP_NUM SMI_LARB_NR
/* vp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vp[SMI_PROFILE_SETTING_COMMON_VP_NUM] = {
	{0, 0x1510}, {0, 0x1169}, {0, 0x1000}, {0, 0x11CE}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vp[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xc}, {0x204, 8}, {0x208, 4}, {0x20c, 0xc}, {0x210, 4}, {0x214, 2}, {0x218, 0xc},
	    {0x21c, 0xc},
	{0x220, 2}, {0x224, 1}, {0x228, 3}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vp[SMI_LARB1_PORT_NUM] = {
	{0x200, 5}, {0x204, 2}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vp[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}, {0x234, 1}, {0x238, 1}, {0x23c,
											     1},
	{0x240, 1}, {0x244, 1}, {0x248, 1}, {0x24c, 1}, {0x250, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb3_vp[SMI_LARB3_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 2}, {0x228, 1}, {0x22c, 1}, {0x230, 4}
};

struct SMI_SETTING vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

/* vr series */
struct SMI_SETTING icfp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING vr_slow_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING vss_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

struct SMI_SETTING venc_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr,
	 smi_profile_setting_larb3_vr}
};

/* vp series */
struct SMI_SETTING vpwfd_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

struct SMI_SETTING swdec_vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp,
	 smi_profile_setting_larb3_vp}
};

/* init seris */
struct SMI_SETTING mm_gpu_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_INITSETTING_LARB0_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init, smi_profile_setting_larb3_init}
};

struct SMI_SETTING ui_idle_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi4k_setting_config = { 0, NULL, {0}, {0} };

#elif defined(SMI_D2)

unsigned long smi_common_l1arb_offset[SMI_LARB_NR] = {
	REG_OFFSET_SMI_L1ARB0, REG_OFFSET_SMI_L1ARB1, REG_OFFSET_SMI_L1ARB2
};

unsigned int smi_dbg_disp_mask = 1;
unsigned int smi_dbg_vdec_mask = 2;
unsigned int smi_dbg_imgsys_mask = 4;
unsigned int smi_dbg_venc_mask = 4;
unsigned int smi_dbg_mjc_mask = 0;

unsigned long smi_larb0_debug_offset[SMI_LARB0_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb1_debug_offset[SMI_LARB1_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb2_debug_offset[SMI_LARB2_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM] = {
	0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x220, 0x230, 0x234, 0x238, 0x400, 0x404, 0x408,
	0x40C, 0x430, 0x440
};

int smi_larb_debug_offset_num[SMI_LARB_NR] = {
	SMI_LARB0_DEBUG_OFFSET_NUM, SMI_LARB1_DEBUG_OFFSET_NUM, SMI_LARB2_DEBUG_OFFSET_NUM
};

unsigned long *smi_larb_debug_offset[SMI_LARB_NR] = {
	smi_larb0_debug_offset, smi_larb1_debug_offset, smi_larb2_debug_offset
};

#define SMI_VC_SETTING_NUM SMI_LARB_NR
struct SMI_SETTING_VALUE smi_vc_setting[SMI_VC_SETTING_NUM] = {
	{0x20, 0}, {0x20, 2}, {0x20, 1}
};

#define SMI_PROFILE_SETTING_COMMON_INIT_NUM 6
/* init_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_init[SMI_PROFILE_SETTING_COMMON_INIT_NUM] = {
	{0, 0}, {0, 0}, {0, 0},
	{0x100, 0xb},
	{0x234, (0x1 << 31) + (0x1d << 26) + (0x1f << 21) + (0x0 << 20) + (0x3 << 15)
	 + (0x4 << 10) + (0x4 << 5) + 0x5},
	{0x230, (0x7 + (0x8 << 3) + (0x7 << 8))}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_init[SMI_LARB0_PORT_NUM] = {
	{0x200, 0x1f}, {0x204, 0x1f}, {0x208, 4}, {0x20c, 6}, {0x210, 4}, {0x214, 1}, {0x218, 1},
	    {0x21c, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_init[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_init[SMI_LARB2_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING init_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init}
};

#define SMI_PROFILE_SETTING_COMMON_ICFP_NUM SMI_LARB_NR
/* icfp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_icfp[SMI_PROFILE_SETTING_COMMON_ICFP_NUM] = {
	{0, 0x11da}, {0, 0x1000}, {0, 0x1318}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_icfp[SMI_LARB0_PORT_NUM] = {
	{0x200, 6}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_icfp[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_icfp[SMI_LARB2_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 2}, {0x214, 4}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING icfp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_ICFP_NUM, smi_profile_setting_common_icfp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_icfp, smi_profile_setting_larb1_icfp,
	 smi_profile_setting_larb2_icfp}
};

#define SMI_PROFILE_SETTING_COMMON_VR_NUM SMI_LARB_NR
/* vr_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vr[SMI_PROFILE_SETTING_COMMON_VR_NUM] = {
	{0, 0x11ff}, {0, 0x1000}, {0, 0x1361}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vr[SMI_LARB0_PORT_NUM] = {
	{0x200, 6}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vr[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vr[SMI_LARB2_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 4}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 2}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING vr_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr}
};

#define SMI_PROFILE_SETTING_COMMON_VP_NUM SMI_LARB_NR
/* vp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vp[SMI_PROFILE_SETTING_COMMON_VP_NUM] = {
	{0, 0x11ff}, {0, 0}, {0, 0x1361}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vp[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 8}, {0x208, 1}, {0x20c, 1}, {0x210, 3}, {0x214, 1}, {0x218, 4}, {0x21c,
											     1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vp[SMI_LARB1_PORT_NUM] = {
	{0x200, 0xb}, {0x204, 0xe}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vp[SMI_LARB2_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 4}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 2}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp}
};

#define SMI_PROFILE_SETTING_COMMON_VPWFD_NUM SMI_LARB_NR
/* vfd_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vpwfd[SMI_PROFILE_SETTING_COMMON_VPWFD_NUM] = {
	{0, 0x11ff}, {0, 0}, {0, 0x1361}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vpwfd[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 8}, {0x208, 1}, {0x20c, 1}, {0x210, 3}, {0x214, 1}, {0x218, 4}, {0x21c,
											     1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vpwfd[SMI_LARB1_PORT_NUM] = {
	{0x200, 0xb}, {0x204, 0xe}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb2_vpwfd[SMI_LARB2_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 4}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 2}, {0x22c, 1}, {0x230, 1}
};

struct SMI_SETTING vpwfd_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VPWFD_NUM, smi_profile_setting_common_vpwfd,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vpwfd, smi_profile_setting_larb1_vpwfd,
	 smi_profile_setting_larb2_vpwfd}
};

/* vp series */
struct SMI_SETTING swdec_vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp, smi_profile_setting_larb2_vp}
};

/* vr series */
struct SMI_SETTING vr_slow_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr}
};

struct SMI_SETTING venc_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr, smi_profile_setting_larb2_vr}
};

/* init series */
struct SMI_SETTING mm_gpu_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init,
	 smi_profile_setting_larb2_init}
};
struct SMI_SETTING vss_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING ui_idle_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi4k_setting_config = { 0, NULL, {0}, {0} };

#elif defined(SMI_R)
unsigned int smi_dbg_disp_mask = 1;
unsigned int smi_dbg_vdec_mask = 0;
unsigned int smi_dbg_imgsys_mask = 2;
unsigned int smi_dbg_venc_mask = 2;
unsigned int smi_dbg_mjc_mask = 0;

unsigned long smi_common_l1arb_offset[SMI_LARB_NR] = {
	REG_OFFSET_SMI_L1ARB0, REG_OFFSET_SMI_L1ARB1
};

unsigned long smi_larb0_debug_offset[SMI_LARB0_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_larb1_debug_offset[SMI_LARB1_DEBUG_OFFSET_NUM] = {
	0x0, 0x8, 0x10, 0x24, 0x50, 0x60, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc, 0xc0,
	    0xc8,
	0xcc, 0x200, 0x204, 0x208, 0x20c, 0x210, 0x214, 0x218, 0x21c, 0x220, 0x224, 0x228, 0x22c,
	    0x230,
	0x234, 0x238, 0x23c, 0x240, 0x244, 0x248, 0x24c, 0x280, 0x284, 0x288, 0x28c, 0x290, 0x294,
	    0x298,
	0x29c, 0x2a0, 0x2a4, 0x2a8, 0x2ac, 0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc,
	    0x2d0,
	0x2d4, 0x2d8, 0x2dc, 0x2e0, 0x2e4, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc
};

unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM] = {
	0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x220, 0x230, 0x234, 0x238, 0x400, 0x404, 0x408,
	0x40C, 0x430, 0x440
};

int smi_larb_debug_offset_num[SMI_LARB_NR] = {
	SMI_LARB0_DEBUG_OFFSET_NUM, SMI_LARB1_DEBUG_OFFSET_NUM
};

unsigned long *smi_larb_debug_offset[SMI_LARB_NR] = {
	smi_larb0_debug_offset, smi_larb1_debug_offset
};

#define SMI_VC_SETTING_NUM SMI_LARB_NR
struct SMI_SETTING_VALUE smi_vc_setting[SMI_VC_SETTING_NUM] = {
	{0x20, 0}, {0x20, 2}
};

#define SMI_PROFILE_SETTING_COMMON_INIT_NUM 5
/* init_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_init[SMI_PROFILE_SETTING_COMMON_INIT_NUM] = {
	{0, 0x14cb}, {0, 0x1001},
	{0x100, 0xb},
	{0x234,
	 (0x1 << 31) + (0x1d << 26) + (0x1f << 21) + (0x0 << 20) + (0x3 << 15)
	 + (0x4 << 10) + (0x4 << 5) + 0x5},
	{0x230, (0x3 + (0x8 << 2) + (0x7 << 7))}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_init[SMI_LARB0_PORT_NUM] = {
	{0x200, 0x1c}, {0x204, 4}, {0x208, 6}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_init[SMI_LARB1_PORT_NUM] = {
	{0x200, 1}, {0x204, 1}, {0x208, 1}, {0x20c, 1}, {0x210, 1}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 1}, {0x224, 1}, {0x228, 1}
};

struct SMI_SETTING init_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init}
};

#define SMI_PROFILE_SETTING_COMMON_VR_NUM SMI_LARB_NR
/* vr_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vr[SMI_PROFILE_SETTING_COMMON_VR_NUM] = {
	{0, 0x122b}, {0, 0x142c}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vr[SMI_LARB0_PORT_NUM] = {
	{0x200, 0xa}, {0x204, 1}, {0x208, 1}, {0x20c, 4}, {0x210, 2}, {0x214, 2}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vr[SMI_LARB1_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 4}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 3}, {0x224, 2}, {0x228, 2}
};

struct SMI_SETTING vr_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr}
};

#define SMI_PROFILE_SETTING_COMMON_VP_NUM SMI_LARB_NR
/* vp_setting */
struct SMI_SETTING_VALUE smi_profile_setting_common_vp[SMI_PROFILE_SETTING_COMMON_VP_NUM] = {
	{0, 0x11ff}, {0, 0}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb0_vp[SMI_LARB0_PORT_NUM] = {
	{0x200, 8}, {0x204, 1}, {0x208, 1}, {0x20c, 3}, {0x210, 1}, {0x214, 4}, {0x218, 1}
};

struct SMI_SETTING_VALUE smi_profile_setting_larb1_vp[SMI_LARB1_PORT_NUM] = {
	{0x200, 8}, {0x204, 6}, {0x208, 1}, {0x20c, 1}, {0x210, 4}, {0x214, 1}, {0x218, 1}, {0x21c,
											     1},
	{0x220, 3}, {0x224, 2}, {0x228, 2}
};

struct SMI_SETTING vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp}
};

/* vp series */
struct SMI_SETTING swdec_vp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VP_NUM, smi_profile_setting_common_vp,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vp, smi_profile_setting_larb1_vp}
};

/* vr series */
struct SMI_SETTING vr_slow_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr}
};

struct SMI_SETTING icfp_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr}
};

struct SMI_SETTING venc_setting_config = {
	SMI_PROFILE_SETTING_COMMON_VR_NUM, smi_profile_setting_common_vr,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_vr, smi_profile_setting_larb1_vr}
};

/* init series */
struct SMI_SETTING mm_gpu_setting_config = {
	SMI_PROFILE_SETTING_COMMON_INIT_NUM, smi_profile_setting_common_init,
	{SMI_LARB0_PORT_NUM, SMI_LARB1_PORT_NUM},
	{smi_profile_setting_larb0_init, smi_profile_setting_larb1_init}
};

struct SMI_SETTING vss_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING vpwfd_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING ui_idle_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi_setting_config = { 0, NULL, {0}, {0} };
struct SMI_SETTING hdmi4k_setting_config = { 0, NULL, {0}, {0} };
#endif

struct SMI_PROFILE_CONFIG smi_profile_config[SMI_PROFILE_CONFIG_NUM] = {
	{SMI_BWC_SCEN_NORMAL, &init_setting_config},
	{SMI_BWC_SCEN_VR, &vr_setting_config},
	{SMI_BWC_SCEN_SWDEC_VP, &swdec_vp_setting_config},
	{SMI_BWC_SCEN_VP, &vp_setting_config},
	{SMI_BWC_SCEN_VR_SLOW, &vr_slow_setting_config},
	{SMI_BWC_SCEN_MM_GPU, &mm_gpu_setting_config},
	{SMI_BWC_SCEN_WFD, &vpwfd_setting_config},
	{SMI_BWC_SCEN_VENC, &venc_setting_config},
	{SMI_BWC_SCEN_ICFP, &icfp_setting_config},
	{SMI_BWC_SCEN_UI_IDLE, &ui_idle_setting_config},
	{SMI_BWC_SCEN_VSS, &vss_setting_config},
	{SMI_BWC_SCEN_FORCE_MMDVFS, &init_setting_config},
	{SMI_BWC_SCEN_HDMI, &hdmi_setting_config},
	{SMI_BWC_SCEN_HDMI4K, &hdmi4k_setting_config}
};

void smi_set_nonconstant_variable(void)
{
#if defined(SMI_D2)
	int i = 0;

	for (i = 0; i < SMI_LARB_NR; ++i) {
		smi_profile_setting_common_init[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_init[i].value = default_val_smi_l1arb[i];
		smi_profile_setting_common_icfp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vr[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vpwfd[i].offset = smi_common_l1arb_offset[i];
	}

	smi_profile_setting_common_vp[1].value = default_val_smi_l1arb[1];
	smi_profile_setting_common_vpwfd[1].value = default_val_smi_l1arb[1];

#elif defined(SMI_D1)
	int i = 0;

	M4U_WriteReg32(LARB2_BASE, 0x24, (M4U_ReadReg32(LARB2_BASE, 0x24) & 0xf7ffffff));
	for (i = 0; i < SMI_LARB_NR; ++i) {
		smi_profile_setting_common_vr[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_init[i].offset = smi_common_l1arb_offset[i];
	}
	smi_profile_setting_common_init[0].value = default_val_smi_l1arb[0];

#elif defined(SMI_D3)
	int i = 0;

	M4U_WriteReg32(LARB2_BASE, 0x24, (M4U_ReadReg32(LARB2_BASE, 0x24) & 0xf7ffffff));
	for (i = 0; i < SMI_LARB_NR; ++i) {
		smi_profile_setting_common_vr[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_icfp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vpwfd[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_init[i].offset = smi_common_l1arb_offset[i];
	}
	smi_profile_setting_common_init[0].value = default_val_smi_l1arb[0];

#elif defined(SMI_J)
	unsigned int smi_val = 0;
	int i = 0;

	smi_val = (M4U_ReadReg32(LARB0_BASE, 0x24) & 0xf7ffffff);
	for (i = 0; i < SMI_LARB_NR; ++i)
		smi_isp_hrt_setting[i].value = smi_val;

	for (i = 0; i < SMI_LARB_NR; ++i) {
		smi_profile_setting_common_vr[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_init[i].offset = smi_common_l1arb_offset[i];
	}
#elif defined(SMI_R)
	int i = 0;

	for (i = 0; i < SMI_LARB_NR; ++i) {
		smi_profile_setting_common_vr[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_vp[i].offset = smi_common_l1arb_offset[i];
		smi_profile_setting_common_init[i].offset = smi_common_l1arb_offset[i];
	}
	smi_profile_setting_common_vp[1].offset = smi_common_l1arb_offset[1];
#endif
}
