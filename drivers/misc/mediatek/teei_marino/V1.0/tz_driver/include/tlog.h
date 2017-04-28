#ifndef TLOG_H
#define TLOG_H


#define TLOG_CONTEXT_LEN		(300)
#define TLOG_MAX_CNT			(50)

#define TLOG_UNUSE				(0)
#define TLOG_INUSE				(1)

#define UT_TLOG_VERSION			(2)
#define UT_TYPE_STRING			(1)
#define MAX_LOG_LEN			(250)
/********************************************
	structures for LOG IRQ handler
 ********************************************/
struct tlog_struct {
	int valid;
	struct work_struct work;
	char context[TLOG_CONTEXT_LEN];
};

/********************************************
	structures for utOS printf
 ********************************************/
struct ut_log_buf_head {
	int version;
	int length;
	int write_pos;
	int reserve;
};

struct ut_log_entry {
	int type;
	char context;
	char reserve;
	char reserve2;
	char reserve3;
};

/********************************************
	structures for uTgate LOG
 ********************************************/
struct utgate_log_head {
	int version;
	int length;
	int write_pos;
	int reserve;
};

/*********************************************
	variables for LOG IRQ handler
 *********************************************/

irqreturn_t tlog_handler(void);
long create_utgate_log_thread(unsigned long tlog_virt_addr, unsigned long buff_size);
long create_tlog_thread(unsigned long tlog_virt_addr, unsigned long buff_size);
void init_tlog_entry(void);

#endif // end of TLOG_H
