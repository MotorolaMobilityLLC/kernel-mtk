#include <linux/kernel.h>
#include <linux/clk.h>
#include <ddp_clkmgr.h>
#include <ddp_pwm_mux.h>

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
		clkid = ULPOSC_D8; /* ULPOSC 29M */
		break;
	case 1:
		clkid = ULPOSC_D2; /* ULPOSC 117M */
		break;
	case 2:
		clkid = UNIVPLL2_D4; /* PLL 104M */
		break;
	case 3:
		clkid = ULPOSC_D3; /* ULPOSC 78M */
		break;
	case 4:
		clkid = -1; /* Bypass config:default 26M */
		break;
	case 5:
		clkid = ULPOSC_D10; /* ULPOSC 23M */
		break;
	case 6:
		clkid = ULPOSC_D4; /* ULPOSC 58M */
		break;
	default:
		clkid = -1;
		break;
	}

	return clkid;
}

