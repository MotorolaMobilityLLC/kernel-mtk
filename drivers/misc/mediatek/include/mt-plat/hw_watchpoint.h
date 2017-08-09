#ifndef __HW_BREAKPOINT_H
#define __HW_BREAKPOINT_H

typedef int (*wp_handler) (phys_addr_t addr);

struct wp_event {
	phys_addr_t virt;
	phys_addr_t phys;
	int type;
	wp_handler handler;
	int in_use;
	int auto_disable;
};

#define WP_EVENT_TYPE_READ 1
#define WP_EVENT_TYPE_WRITE 2
#define WP_EVENT_TYPE_ALL 3

#define init_wp_event(__e, __v, __p, __t, __h)   \
	do {    \
		(__e)->virt = (__v);    \
		(__e)->phys = (__p);    \
		(__e)->type = (__t);    \
		(__e)->handler = (__h);    \
		(__e)->auto_disable = 0;    \
	} while (0)

#define auto_disable_wp(__e)   \
	{\
		(__e)->auto_disable = 1; \
	}

extern int add_hw_watchpoint(struct wp_event *wp_event);
extern int del_hw_watchpoint(struct wp_event *wp_event);

#endif				/* !__HW_BREAKPOINT_H */
