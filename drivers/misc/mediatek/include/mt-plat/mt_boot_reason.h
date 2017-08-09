#ifndef __MT_BOOT_REASON_H__
#define __MT_BOOT_REASON_H__

enum boot_reason_t {
	BR_POWER_KEY = 0,
	BR_USB,
	BR_RTC,
	BR_WDT,
	BR_WDT_BY_PASS_PWK,
	BR_TOOL_BY_PASS_PWK,
	BR_2SEC_REBOOT,
	BR_UNKNOWN
};

extern enum boot_reason_t get_boot_reason(void);

#endif
