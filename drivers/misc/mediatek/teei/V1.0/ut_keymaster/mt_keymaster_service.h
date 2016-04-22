#ifndef __MT_KEYMASTER_SERVICE_H__
#define __MT_KEYMASTER_SERVICE_H__

extern unsigned long mt_keymaster_message_buf;
extern unsigned long mt_keymaster_fmessage_buf;
extern unsigned long mt_keymaster_bmessage_buf;

long create_keymaster_cmd_buf(void);
#endif
