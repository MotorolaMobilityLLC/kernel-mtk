/**
* @file    mt_clk_buf_ctl.c
* @brief   Driver for RF clock buffer control
*
*/
#ifndef __MT_CLK_BUF_CTL_H__
#define __MT_CLK_BUF_CTL_H__

#include <linux/kernel.h>
#include <linux/mutex.h>
#if defined(CONFIG_MTK_LEGACY)
#include <cust_clk_buf.h>
#endif
#if 0
#define GPIO53 53
#define GPIO54 54
#define GPIO55 55
#define GPIO56 56
#define GPIO57 57
#endif

#if 1 /* TODO, need add Pad name@DCT tool */
#ifndef GPIO_RFIC0_BSI_CS
#define GPIO_RFIC0_BSI_CS         (GPIO110|0x80000000)    /* RFIC0_BSI_CS = GPIO53 */
#endif
#ifndef GPIO_RFIC0_BSI_CK
#define GPIO_RFIC0_BSI_CK         (GPIO111|0x80000000)    /* RFIC0_BSI_CK = GPIO54 */
#endif
#ifndef GPIO_RFIC0_BSI_D0
#define GPIO_RFIC0_BSI_D0         (GPIO114|0x80000000)    /* RFIC0_BSI_D0 = GPIO55 */
#endif
#ifndef GPIO_RFIC0_BSI_D1
#define GPIO_RFIC0_BSI_D1         (GPIO113|0x80000000)    /* RFIC0_BSI_D1 = GPIO56 */
#endif
#ifndef GPIO_RFIC0_BSI_D2
#define GPIO_RFIC0_BSI_D2         (GPIO112|0x80000000)    /* RFIC0_BSI_D2 = GPIO57 */
#endif
#endif


enum clk_buf_id {
	CLK_BUF_BB_MD		= 0,
	CLK_BUF_CONN		= 1,
	CLK_BUF_NFC			= 2,
	CLK_BUF_AUDIO		= 3,
	CLK_BUF_INVALID		= 4,
};
#if !defined(CONFIG_MTK_LEGACY)
typedef enum {
	CLK_BUF_DISABLE	= 0,
	CLOCK_BUFFER_SW_CONTROL	= 1,
	CLOCK_BUFFER_HW_CONTROL	= 2,
} CLK_BUF_STATUS;
#endif
typedef enum {
	CLK_BUF_SW_DISABLE = 0,
	CLK_BUF_SW_ENABLE  = 1,
} CLK_BUF_SWCTRL_STATUS_T;
#define CLKBUF_NUM         4

#define STA_CLK_ON      1
#define STA_CLK_OFF     0

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff);
void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status);
bool clk_buf_init(void);

extern struct mutex clk_buf_ctrl_lock;
extern struct kobject *power_kobj;
#endif

