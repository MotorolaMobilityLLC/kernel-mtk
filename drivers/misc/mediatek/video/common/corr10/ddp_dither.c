#include <linux/kernel.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
/* #include <ddp_clkmgr.h> */
#endif
#include <cmdq_record.h>
#include <ddp_reg.h>
#include <ddp_path.h>
#include <ddp_dither.h>
#include <ddp_drv.h>

int dither_dbg_en = 0;
#define DITHER_ERR(fmt, arg...) pr_err("[DITHER] " fmt "\n", ##arg)
#define DITHER_DBG(fmt, arg...) \
	do { if (dither_dbg_en) pr_debug("[DITHER] " fmt "\n", ##arg); } while (0)

#define DITHER_REG(reg_base, index) ((reg_base) + 0x100 + (index) * 4)

void disp_dither_init(disp_dither_id_t id, int width, int height,
			     unsigned int dither_bpp, void *cmdq)
{
	unsigned long reg_base = DISPSYS_DITHER_BASE;
	unsigned int enable;

	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 5), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 6), 0x00003004, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 7), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 8), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 9), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 10), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 11), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 12), 0x00000011, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 13), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 14), 0x00000000, ~0);

	enable = 0x1;
	if (dither_bpp == 16) {	/* 565 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x50500001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x50504040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 18) {	/* 666 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x40400001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x40404040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 24) {	/* 888 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x20200001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x20202020, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp > 24) {
		DITHER_DBG("High depth LCM (bpp = %d), no dither\n", dither_bpp);
		enable = 1;
	} else {
		DITHER_DBG("invalid dither bpp = %d\n", dither_bpp);
		/* Bypass dither */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000000, ~0);
		enable = 0;
	}

	DISP_REG_MASK(cmdq, DISP_REG_DITHER_EN, enable, 0x1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG, enable << 1, 1 << 1);
	DISP_REG_SET(cmdq, DISP_REG_DITHER_SIZE, (width << 16) | height);
}


static int disp_dither_config(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *cmdq)
{
	if (pConfig->dst_dirty) {
		disp_dither_init(DISP_DITHER0, pConfig->dst_w, pConfig->dst_h,
				 pConfig->lcm_bpp, cmdq);
	}

	return 0;
}


static int disp_dither_bypass(DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass)
		relay = 1;

	DISP_REG_MASK(NULL, DISP_REG_DITHER_CFG, relay, 0x1);

	DITHER_DBG("disp_dither_bypass(bypass = %d)", bypass);

	return 0;
}


static int disp_dither_power_on(DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	if (module == DISP_MODULE_DITHER) {
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_DITHER, "DITHER");
#else
#if defined(CONFIG_ARCH_MT6755)
		/* ddp_clk_enable(DISP0_DISP_DITHER); */
#else
		ddp_clk_enable(DISP0_DISP_DITHER);
#endif
#endif
	}
#endif
	return 0;
}

static int disp_dither_power_off(DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	if (module == DISP_MODULE_DITHER) {
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_DITHER, "DITHER");
#else
#if defined(CONFIG_ARCH_MT6755)
		/* ddp_clk_disable(DISP0_DISP_DITHER); */
#else
		ddp_clk_disable(DISP0_DISP_DITHER);
#endif
#endif
	}
#endif
	return 0;
}



DDP_MODULE_DRIVER ddp_driver_dither = {
	.config = disp_dither_config,
	.bypass = disp_dither_bypass,
	.init = disp_dither_power_on,
	.deinit = disp_dither_power_off,
	.power_on = disp_dither_power_on,
	.power_off = disp_dither_power_off,
};


void disp_dither_select(unsigned int dither_bpp, void *cmdq)
{
	unsigned long reg_base = DISPSYS_DITHER_BASE;
	unsigned int enable;

	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 5), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 6), 0x00003004, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 7), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 8), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 9), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 10), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 11), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 12), 0x00000011, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 13), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 14), 0x00000000, ~0);

	enable = 0x1;
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_0, 0 << 4, 1 << 4);
	if (dither_bpp == 16) {	/* 565 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x50500001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x50504040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 18) {	/* 666 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x40400001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x40404040, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp == 24) {	/* 888 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x20200001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x20202020, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000001, ~0);
	} else if (dither_bpp > 24) {
		DITHER_DBG("[DITHER] High depth LCM (bpp = %d), no dither\n", dither_bpp);
		enable = 1;
	} else {
		DITHER_DBG("[DITHER] invalid dither bpp = %d\n", dither_bpp);
		/* Bypass dither */
		DISP_REG_MASK(cmdq, DISP_REG_DITHER_0, 1 << 4, 1 << 4);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 0), 0x00000000, ~0);
		enable = 0;
	}
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_EN, enable, 0x1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG, enable << 1, 1 << 1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG, 0x0, 0x1);
}


void disp_dither_dump(void)
{
}


void dither_test(const char *cmd, char *debug_output)
{
	debug_output[0] = '\0';
	DITHER_DBG("[DITHER]dither_test(%s)", cmd);

	if (strncmp(cmd, "sel:", 4) == 0) {
		if (cmd[4] == '0') {
			disp_dither_select(0, NULL);
			DITHER_DBG("[DITHER] bbp=0");
		} else if (cmd[4] == '1') {
			disp_dither_select(16, NULL);
			DITHER_DBG("[DITHER] bbp=16");
		} else if (cmd[4] == '2') {
			disp_dither_select(18, NULL);
			DITHER_DBG("[DITHER] bbp=18");
		} else if (cmd[4] == '3') {
			disp_dither_select(24, NULL);
			DITHER_DBG("[DITHER] bbp=24");
		} else {
		}
	}
}
