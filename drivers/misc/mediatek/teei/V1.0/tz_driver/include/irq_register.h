#ifndef IRQ_REGISTER_H
#define IRQ_REGISTER_H

#define SCHED_ENT_CNT  10

extern struct service_handler reetime;
extern struct service_handler socket;
extern struct service_handler vfs_handler;
extern struct service_handler printer_driver;
#ifdef TUI_SUPPORT
extern struct semaphore tui_notify_sema;
#endif

struct work_entry {
	int call_no;
	int in_use;
	struct work_struct work;
};

struct load_soter_entry {
	unsigned long vfs_addr;
	struct work_struct work;
};

int register_switch_irq_handler(void);
void load_func(struct work_struct *entry);
void work_func(struct work_struct *entry);
void init_sched_work_ent(void);
int register_ut_irq_handler(int irq);
int register_soter_irq_handler(int irq);
long init_all_service_handlers(void);
int vfs_thread_function(unsigned long virt_addr, unsigned long para_vaddr, unsigned long buff_vaddr);
void secondary_load_func(void);

#endif /* end of IRQ_REGISTER_H */
