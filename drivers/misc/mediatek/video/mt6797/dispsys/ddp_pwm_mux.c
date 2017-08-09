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
	case 1:
		clkid = UNIVPLL2_D4;
		break;
	case 2:
		clkid = ULPOSC_D2;
		break;
	case 3:
		clkid = ULPOSC_D3;
		break;
	case 4:
		clkid = ULPOSC_D8;
		break;
	case 5:
		clkid = ULPOSC_D10;
		break;
	case 6:
		clkid = ULPOSC_D4;
		break;
	default:
		clkid = -1;
		break;
	}

	return clkid;
}

