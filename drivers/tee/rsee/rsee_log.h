#ifndef __RSEE_LOG_H__
#define __RSEE_LOG_H__




#define 	DEBUG_MSG_TYPE_CORE  	0
#define 	DEBUG_MSG_TYPE_TA 		1
#define 	DEBUG_MSG_TYPE_COUNT 	2


typedef struct
{
	uint32_t msg_type;
	uint32_t msg_len;
}dbg_log_header_t;

int tee_log_add(char *log, int len);
int tee_log_init(void);
int tee_log_remove(void);

#endif