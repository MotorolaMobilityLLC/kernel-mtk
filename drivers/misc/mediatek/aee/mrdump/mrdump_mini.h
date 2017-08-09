#if !defined(__MRDUMP_MINI_H__)
int mrdump_mini_init(void);
extern raw_spinlock_t logbuf_lock;
extern unsigned long *stack_trace;
extern void get_kernel_log_buffer(unsigned long *addr, unsigned long *size, unsigned long *start);
/* extern void get_android_log_buffer(unsigned long *addr, unsigned long *size, unsigned long *start, int type); */
extern struct ram_console_buffer *ram_console_buffer;
#endif
