#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <ddp_clkmgr.h>
#include <ddp_pwm_mux.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <ddp_reg.h>

/*****************************************************************************
 *
 * disp pwm source clock select mux api
 *
*****************************************************************************/
eDDP_CLK_ID disp_pwm_get_clkid(unsigned int clk_req)
{
	eDDP_CLK_ID clkid = -1;

	switch (clk_req) {
	case 0:
		clkid = ULPOSC_D8;
		break;
	case 1:
		clkid = ULPOSC_D2;
		break;
	case 2:
		clkid = UNIVPLL2_D4;
		break;
	default:
		clkid = -1;
		break;
	}

	return clkid;
}

#define PWM_ERR(fmt, arg...) pr_err("[PWM] " fmt "\n", ##arg)
#define PWM_NOTICE(fmt, arg...) pr_warn("[PWM] " fmt "\n", ##arg)
#define PWM_MSG(fmt, arg...) pr_debug("[PWM] " fmt "\n", ##arg)

static void __iomem *disp_pmw_osc_base;

#ifndef OSC_ULPOSC_ADDR /* rosc control register address */
#define OSC_ULPOSC_ADDR (disp_pmw_osc_base + 0x458)
#endif

/* clock hard code access API */
#define DRV_Reg32(addr) INREG32(addr)
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)

/*****************************************************************************
 *
 * get disp pwm source osc
 *
*****************************************************************************/
static int disp_pwm_get_oscbase(void)
{
	int ret = 0;
	struct device_node *node;

	if (disp_pmw_osc_base != NULL) {
		PWM_MSG("SLEEP node exist");
		return 0;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node) {
		PWM_ERR("DISP find SLEEP node failed\n");
		return -1;
	}
	disp_pmw_osc_base = of_iomap(node, 0);
	if (!disp_pmw_osc_base) {
		PWM_ERR("DISP find SLEEP base failed\n");
		return -1;
	}
	PWM_MSG("find SLEEP node");
	return ret;
}

static int disp_pwm_check_osc_status(void)
{
	unsigned int regosc;
	int ret = 0;

	regosc = clk_readl(OSC_ULPOSC_ADDR);
	if ((regosc & 0x5) != 0x5) {
		PWM_MSG("osc is off (%x)", regosc);
		ret = 0;
	} else {
		PWM_MSG("osc is on (%x)", regosc);
		ret = 1;
	}

	return ret;
}

/*****************************************************************************
 *
 * hardcode turn on/off ROSC api
 *
*****************************************************************************/
int disp_pwm_osc_on(void)
{
	unsigned int regosc;

	if (disp_pwm_get_oscbase() == -1)
		return -1;

	if (disp_pwm_check_osc_status() == 1)
		return 0; /* osc already on */

	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x", regosc);

	/* OSC EN = 1 */
	regosc = regosc | 0x1;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after en", regosc);
	udelay(11);

	/* OSC RST  */
	regosc = regosc | 0x2;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after rst 1", regosc);
	udelay(40);
	regosc = regosc & 0xfffffffd;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after rst 0", regosc);
	udelay(130);

	/* OSC CG_EN = 1 */
	regosc = regosc | 0x4;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after cg_en", regosc);

	return 0;

}

int disp_pwm_osc_off(void)
{
	unsigned int regosc;

	if (disp_pwm_get_oscbase() == -1)
		return -1;

	regosc = clk_readl(OSC_ULPOSC_ADDR);

	/* OSC CG_EN = 0 */
	regosc = regosc & (~0x4);
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after cg_en", regosc);

	udelay(40);

	/* OSC EN = 0 */
	regosc = regosc & (~0x1);
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	PWM_MSG("ULPOSC config : 0x%08x after en", regosc);

	return 0;
}

