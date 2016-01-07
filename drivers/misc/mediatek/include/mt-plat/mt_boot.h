#ifndef __MT_BOOT_H__
#define __MT_BOOT_H__
#include <mt_boot_common.h>
#include <mt_boot_reason.h>
#include <mt_chip.h>

/*META COM port type*/
enum meta_com_type {
	META_UNKNOWN_COM = 0,
	META_UART_COM,
	META_USB_COM
};

extern enum meta_com_type get_meta_com_type(void);
extern unsigned int get_meta_com_id(void);
extern unsigned int get_meta_uart_port(void);

#endif
