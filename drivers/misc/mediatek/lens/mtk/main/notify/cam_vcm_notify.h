#ifndef _LINUX_CAM_VCM_H
#define _LINUX_CAM_VCM_H

#include <linux/kgdb.h>


#include <linux/fs.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <linux/backlight.h>
#include <linux/slab.h>
#include <asm/io.h>

#define CAM_VCM_EVENT_REPORT            0x0f
#define CAM_VCM_EVENT_VIB_ON		0x00
#define CAM_VCM_EVENT_VIB_OFF		0x01

struct cam_vcm_event {
	void *data;
};

extern int cam_vcm_register_client(struct notifier_block *nb);
extern int cam_vcm_unregister_client(struct notifier_block *nb);
extern int cam_vcm_notifier_call_chain(unsigned long val, void *v);

#endif /* _LINUX_CAM_VCM_H */
