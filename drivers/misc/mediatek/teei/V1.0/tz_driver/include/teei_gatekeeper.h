#ifndef __TEEI_GATEKEEPER_H__
#define __TEEI_GATEKEEPER_H__

struct gatekeeper_command_struct {
	unsigned long mem_size;
	int retVal;
};

extern unsigned long gatekeeper_buff_addr;
//extern struct semaphore gatekeeper_lock;
extern struct mutex pm_mutex;

unsigned long create_gatekeeper_fdrv(int buff_size);
int __send_gatekeeper_command(unsigned long share_memory_size);

#endif /*__TEEI_GATEKEEPER_H__*/
