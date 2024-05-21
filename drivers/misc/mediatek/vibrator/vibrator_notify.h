#ifndef _LINUX_VIBRATOR_H
#define _LINUX_VIBRATOR_H

#include <linux/kgdb.h>


#include <linux/fs.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <linux/backlight.h>
#include <linux/slab.h>
#include <asm/io.h>

#define VIBRATOR_EVENT_REPORT                  0xf
#define CAM_EVENT_OFF                 0x00
#define CAM_EVENT_ON                 0x01
/*	NEAR BY */
#define VIBRATOR_EVENT_NEAR_BY		0x00
/*FAR AWAY*/
#define VIBRATOR_EVENT_FAR_AWAY		0x01

struct vibrator_event {
	void *data;
};

extern int vibrator_register_client(struct notifier_block *nb);
extern int vibrator_unregister_client(struct notifier_block *nb);
extern int vibrator_notifier_call_chain(unsigned long val, void *v);

#endif /* _LINUX_VIBRATOR_H */