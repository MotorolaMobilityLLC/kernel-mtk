#ifndef TEEI_KEYMASTER_HH
#define TEEI_KEYMASTER_HH

struct keymaster_command_struct {
	unsigned long mem_size;
	int retVal;
};

extern struct keymaster_command_struct keymaster_command_entry;
extern unsigned long keymaster_buff_addr;
extern struct mutex pm_mutex;

unsigned long create_keymaster_fdrv(int buff_size);
int __send_keymaster_command(unsigned long share_memory_size);

#endif /* end of TEEI_KEYMASTER_HH */
