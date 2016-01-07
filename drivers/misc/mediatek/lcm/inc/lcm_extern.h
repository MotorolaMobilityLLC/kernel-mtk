#ifndef __LCM_EXT_H__
#define __LCM_EXT_H__

extern struct i2c_client *tps65132_i2c_client;
extern int tps65132_write_bytes(unsigned char addr, unsigned char value);

#endif
