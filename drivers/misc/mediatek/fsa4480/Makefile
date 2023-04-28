# add -Wall to try to catch everything we can.
EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -I$(TOP)/kernel-5.10/include
EXTRA_CFLAGS += -I$(TOP)/kernel-5.10/include/uapi/

ifeq ($(CONFIG_FSA4480_SENS_TO_GND), y)
    EXTRA_CFLAGS += -DCONFIG_FSA4480_SENS_TO_GND
endif

fsa4480-objs = fsa4480-i2c.o
obj-$(CONFIG_TYPEC_FSA4480) += fsa4480.o
