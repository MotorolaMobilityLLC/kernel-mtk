#include "dw9781_i2c.h"

//#define DW9781_DEBUG
#ifdef DW9781_DEBUG
#define LOG_INF(format, args...) \
	printk(" [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif
#define DW9781_I2C_SLAVE_ADDR 0xE4

static struct i2c_client *dw9781_i2c_client = NULL;

void dw9781_set_i2c_client(struct i2c_client *i2c_client)
{
	dw9781_i2c_client = i2c_client;
}

int write_reg_16bit_value_16bit(unsigned short regAddr, unsigned short regData)
{
	int i4RetValue = 0;
	struct i2c_msg msgs;
	char puSendCmd[4] = { (char)(regAddr>>8), (char)(regAddr&0xff),
	                      (char)(regData >> 8),
	                      (char)(regData&0xFF) };

	LOG_INF("DW9781 I2C read:%04x, data:%04x", regAddr, regData);

	if (!dw9781_i2c_client) {
		printk("[%s] FATAL: DW9781 i2c client is NULL!!!",__func__);
		return -1;
	}

	msgs.addr  = DW9781_I2C_SLAVE_ADDR >> 1;
	msgs.flags = 0;
	msgs.len   = 4;
	msgs.buf   = puSendCmd;

	i4RetValue = i2c_transfer(dw9781_i2c_client->adapter, &msgs, 1);
	if (i4RetValue != 1) {
		printk("I2C send failed!!\n");
		return -1;
	}

	return 0;
}

int read_reg_16bit_value_16bit(unsigned short regAddr, unsigned short *regData)
{
	int i4RetValue = 0;
	unsigned short readTemp = 0;
	struct i2c_msg msg[2];
	char puReadCmd[2] = { (char)(regAddr>>8), (char)(regAddr&0xff)};

	LOG_INF("DW9781 I2C read:%04x", regAddr);

	if (!dw9781_i2c_client) {
		printk("[%s] FATAL: DW9781 i2c client is NULL!!!",__func__);
		return -1;
	}
	dw9781_i2c_client->addr = DW9781_I2C_SLAVE_ADDR >> 1;

	msg[0].addr = dw9781_i2c_client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = puReadCmd;

	msg[1].addr = dw9781_i2c_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = (u8*)&readTemp;

	i4RetValue = i2c_transfer(dw9781_i2c_client->adapter, msg, ARRAY_SIZE(msg));

	if (i4RetValue != 2) {
		printk("DW9781 I2C read failed!!\n");
		return -1;
	}

	*regData = ((readTemp>>8) & 0xff) | ((readTemp<<8) & 0xff00);

	return 0;
}

void i2c_block_write_reg(unsigned short addr,unsigned short *fwContentPtr,int size)
{
	char puSendCmd[DATPKT_SIZE*2 + 2];
	int IDX = 0;
	int tosend = 0;
	u16 data;
	puSendCmd[tosend++] = (char)(addr >> 8);
	puSendCmd[tosend++] = (char)(addr & 0xff);
	while(size > IDX) {
		data = fwContentPtr[IDX];
		puSendCmd[tosend++] = (char)(data >> 8);
		puSendCmd[tosend++] = (char)(data & 0xff);
		IDX++;
		if(IDX == size) {
			int i4RetValue = 0;
			struct i2c_msg msgs;
			msgs.addr  = DW9781_I2C_SLAVE_ADDR >> 1;
			msgs.flags = 0;
			msgs.len   = (DATPKT_SIZE * 2 + 2);
			msgs.buf   = puSendCmd;
			i4RetValue = i2c_transfer(dw9781_i2c_client->adapter, &msgs, 1);
			if (i4RetValue != 1) {
				LOG_INF("I2C send failed!!\n");
				return;
			}
			return;
		}
	}
}

void i2c_block_read_reg(unsigned short addr,unsigned short *temp,int size)
{
	int i4RetValue = 0;
	char puSendCmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };
	struct i2c_msg msgs[2];

	msgs[0].addr  = DW9781_I2C_SLAVE_ADDR >> 1;
	msgs[0].flags = 0;
	msgs[0].len   = 2;
	msgs[0].buf   = puSendCmd;

	msgs[1].addr  = DW9781_I2C_SLAVE_ADDR >> 1;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len   = 2 * size;
	msgs[1].buf   = (u8*)temp;

	i4RetValue = i2c_transfer(dw9781_i2c_client->adapter, msgs, 2);
	if (i4RetValue != 2) {
		LOG_INF("I2C send failed!!\n");
		return;
	}
	return;
}
