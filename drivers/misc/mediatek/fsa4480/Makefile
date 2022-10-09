# add -Wall to try to catch everything we can.
EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -I$(TOP)/kernel-4.19/include
EXTRA_CFLAGS += -I$(TOP)/kernel-4.19/include/uapi/

fsa4480-objs = fsa4480-i2c.o
obj-$(CONFIG_TYPEC_FSA4480) += fsa4480.o
