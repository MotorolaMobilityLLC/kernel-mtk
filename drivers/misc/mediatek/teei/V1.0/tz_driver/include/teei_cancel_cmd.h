#ifndef TEEI_CANCEL_CMD_H
#define TEEI_CANCEL_CMD_H

extern unsigned long cancel_message_buff;
extern struct semaphore fp_lock;
extern struct mutex pm_mutex;

unsigned long create_cancel_fdrv(int buff_size);
int send_cancel_command(unsigned long share_memory_size);
int __send_cancel_command(unsigned long share_memory_size);

#endif /* end of TEEI_CANCEL_CMD_H */
