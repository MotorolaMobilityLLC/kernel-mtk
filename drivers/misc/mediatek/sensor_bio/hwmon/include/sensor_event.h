#ifndef _SENSOR_EVENT_H
#define _SENSOR_EVENT_H
#include <linux/poll.h>

#define CONTINUE_SENSOR_BUF_SIZE	2048
#define OTHER_SENSOR_BUF_SIZE	16
struct sensor_event {
	int64_t time_stamp;
	int8_t handle;
	int8_t flush_action;
	int8_t status;
	int8_t reserved;
	union {
		int32_t word[6];
		int8_t byte[0];
	};
} __packed;
ssize_t sensor_event_read(unsigned char handle, struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos);
unsigned int sensor_event_poll(unsigned char handle, struct file *file, poll_table *wait);
int sensor_input_event(unsigned char handle,
			 const struct sensor_event *event);
unsigned int sensor_event_register(unsigned char handle);
unsigned int sensor_event_deregister(unsigned char handle);
#endif
