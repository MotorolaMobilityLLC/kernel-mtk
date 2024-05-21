###
# Copyright (C) Fourier Semiconductor Inc. 2016-2021. All rights reserved.
#

# TOPLEVEL=$(PWD)/..
# EXTRA_CFLAGS += -I$(TOPLEVEL)

# EXTRA_CFLAGS += -DDEBUG -DFSM_DEBUG
EXTRA_CFLAGS += -Wall -Werror
MODFLAGS = -fno-pic
CFLAGS_MODULE = $(MODFLAGS)
AFLAGS_MODULE = $(MODFLAGS)

snd-soc-fs1599-objs := fsm_core.o
snd-soc-fs1599-objs += fs15xx.o
snd-soc-fs1599-objs += fsm_i2c.o
obj-y += snd-soc-fs1599.o
