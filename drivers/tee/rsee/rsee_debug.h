#ifndef __RSEE_DEBUG_H__
#define __RSEE_DEBUG_H__

typedef enum
{
	DEBUG_MSG_TYPE_CORE = 0,
	DEBUG_MSG_TYPE_TA,
	DEBUG_MSG_TYPE_COUNT
}dbg_msg_t;

typedef struct
{
	dbg_msg_t msg_type;
	size_t msg_len;
}dbg_log_header_t;

#endif