#define pr_fmt(fmt) "mt8193-gpio: " fmt

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "mt8193_pinmux.h"
#include "mt8193_gpio.h"
#include "mt8193.h"


/* config gpio input/output low/high */
int GPIO_Config(u32 u4GpioNum, u8 u1Mode, u8 u1Value)
{
	int ret = 0;
	u32 u4PadNum = 0;
	u32 u4Tmp = 0;

	if (u4GpioNum > GPIO_PIN_MAX) {
		pr_err("[PINMUX] Invalid GPIO NUM %d\n", u4GpioNum);
		return PINMUX_RET_INVALID_ARG;
	}

	u4PadNum = _au4GpioTbl[u4GpioNum];

	pr_debug("[PINMUX] GPIO_Config() %d, %d, %d, %d\n", u4GpioNum, u4PadNum, u1Mode, u1Value);

	if (u4PadNum >= PIN_NFD6 && u4PadNum <= PIN_NFD0) {
		/* NFI ND GROUP. function 1 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION1);
	} else if (u4PadNum >= PIN_G0 && u4PadNum <= PIN_VSYNC) {
		/* GRB OUT GROUP. function 0 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION0);
	} else if (u4PadNum >= PIN_CEC && u4PadNum <= PIN_HTPLG) {
		/* HDMI GROUP. function 2 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum >= PIN_I2S_DATA && u4PadNum <= PIN_I2S_BCK) {
		/* audio GROUP. function 3 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION3);
	} else if (u4PadNum ==  PIN_DPI1CK) {
		/* RGB_IN2 group. function 2 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum >= PIN_DPI1D7 && u4PadNum <= PIN_DPI1D0) {
		/* RGB_IN1 GROUP. function 3 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION3);
	} else if (u4PadNum >= PIN_DPI0CK && u4PadNum <= PIN_DPI0D3) {
		/* RGB_IN1 GROUP. function 2 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum >= PIN_DPI0D2 && u4PadNum <= PIN_DPI0D0) {
		/* RGB_IN1 GROUP. function 1 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION1);
	} else if (u4PadNum >= PIN_SDA && u4PadNum <= PIN_SCL) {
		/* I2C_SLAVE GROUP. function 1 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION1);
	} else if (u4PadNum >= PIN_NRNB && u4PadNum <= PIN_NLD7) {
		/* NFI_6583 GROUP. function 2 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum >= PIN_NLD6 && u4PadNum <= PIN_NLD3) {
		/* NFI_6583 GROUP. function 3 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION3);
	} else if (u4PadNum >= PIN_NLD2 && u4PadNum <= PIN_NLD0) {
		/* NFI_6583 GROUP. function 2 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum ==  PIN_INT) {
		/* INTERRUPT group. function 3 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION3);
	} else if (u4PadNum >= PIN_NLD2 && u4PadNum <= PIN_NLD0) {
		/* NFI_6583 GROUP. function 2 is gpio */
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else if (u4PadNum ==  PIN_NFRBN) {
		/* NFI_ND group. function 1 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION1);
	} else if (u4PadNum >= PIN_NFCLE && u4PadNum <= PIN_NFALE) {
		/* NFI ND GROUP. function 3 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION3);
	} else if (u4PadNum >= PIN_NFWEN && u4PadNum <= PIN_NFD7) {
		/* NFI ND GROUP. function 2 is gpio*/
		ret = mt8193_pinset(u4PadNum, PINMUX_FUNCTION2);
	} else {
		pr_err("[PINMUX] GPIO CFG ERROR !\n");
		return PINMUX_RET_INVALID_ARG;
	}

	if (u1Mode == MT8193_GPIO_OUTPUT) {
		/* output*/
		if (u4GpioNum >= 32 && u4GpioNum <= 63) {
			u4Tmp = CKGEN_READ32(REG_RW_GPIO_EN_1);
			u4Tmp |= (1 << (u4GpioNum-32));
			CKGEN_WRITE32(REG_RW_GPIO_EN_1, u4Tmp);

			u4Tmp = CKGEN_READ32(REG_RW_GPIO_OUT_1);

			if (u1Value == MT8193_GPIO_HIGH)
				u4Tmp |= (1 << (u4GpioNum-32));
			else
				u4Tmp &= (~(1 << (u4GpioNum-32)));

			CKGEN_WRITE32(REG_RW_GPIO_OUT_1, u4Tmp);
		} else if (u4GpioNum >= 0 && u4GpioNum <= 31) {
			/* work around method for gpio0~gpio31 */
			u32 u4PdShift = 0;
			u32 u4PuShift = 0;

			u4PdShift = _au4PinmuxPadPuPdTbl[u4PadNum][0];
			u4PuShift = _au4PinmuxPadPuPdTbl[u4PadNum][1];

			u4Tmp = CKGEN_READ32(REG_RW_GPIO_EN_0);

			if (u1Value == MT8193_GPIO_HIGH) {
				/* for output high, set en as 0. set pad pd as 0, set pad pu as 1 */
				u4Tmp &= (~(1 << u4GpioNum));
				CKGEN_WRITE32(REG_RW_GPIO_EN_0, u4Tmp);

				if (u4PdShift <= 31) {
					u4Tmp = CKGEN_READ32(REG_RW_PAD_PD0);
					u4Tmp &= (~(1 << u4PdShift));
					CKGEN_WRITE32(REG_RW_PAD_PD0, u4Tmp);

					u4Tmp = CKGEN_READ32(REG_RW_PAD_PU0);
					u4Tmp |= (1 << u4PdShift);
					CKGEN_WRITE32(REG_RW_PAD_PU0, u4Tmp);
				} else if (u4PdShift >= 32 && u4PdShift <= 63) {
					u4Tmp = CKGEN_READ32(REG_RW_PAD_PD1);
					u4Tmp &= (~(1 << (u4PdShift - 31)));
					CKGEN_WRITE32(REG_RW_PAD_PD1, u4Tmp);

					u4Tmp = CKGEN_READ32(REG_RW_PAD_PU1);
					u4Tmp |= (1 << (u4PdShift - 31));
					CKGEN_WRITE32(REG_RW_PAD_PU1, u4Tmp);
				}
			} else {
				/* for output low, just set en as 1 */
				u4Tmp |= (1 << u4GpioNum);
				CKGEN_WRITE32(REG_RW_GPIO_EN_0, u4Tmp);
			}

			/* CKGEN_WRITE32(REG_RW_GPIO_OUT_0, u4Tmp); */
		}
	} else {
		/* input*/
		if (u4GpioNum >= 32 && u4GpioNum <= 63) {
			u4Tmp = CKGEN_READ32(REG_RW_GPIO_EN_1);
			u4Tmp &= (~(1 << (u4GpioNum-32)));
			CKGEN_WRITE32(REG_RW_GPIO_EN_1, u4Tmp);
		} else if (u4GpioNum >= 0 && u4GpioNum <= 31) {
			u4Tmp = CKGEN_READ32(REG_RW_GPIO_EN_0);
			u4Tmp &= (~(1 << u4GpioNum));
			CKGEN_WRITE32(REG_RW_GPIO_EN_0, u4Tmp);
		}
	}

	return PINMUX_RET_OK;
}

/* return MT8193_GPIO_HIGH or MT8193_GPIO_LOW */
u8 GPIO_Input(u32 u4GpioNum)
{
	u32 u4Tmp = 0;
	u8  u1Val = MT8193_GPIO_LOW;

	if (u4GpioNum >= 32 && u4GpioNum <= 63) {
		u4Tmp = CKGEN_READ32(REG_RW_GPIO_IN_1);
		u1Val = (u4Tmp & (1 << (u4GpioNum-32))) ? MT8193_GPIO_HIGH : MT8193_GPIO_LOW;
	} else if (u4GpioNum >= 0 && u4GpioNum <= 31) {
		u4Tmp = CKGEN_READ32(REG_RW_GPIO_IN_0);
		u1Val = (u4Tmp & (1 << u4GpioNum)) ? MT8193_GPIO_HIGH : MT8193_GPIO_LOW;
	}

	return u1Val;
}

int GPIO_Output(u32 u4GpioNum, u32 u4High)
{
	int ret = 0;

	ret = GPIO_Config(u4GpioNum, MT8193_GPIO_OUTPUT, u4High);

	return ret;
}
