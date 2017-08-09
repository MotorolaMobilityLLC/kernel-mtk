obj-y += hw_watchpoint_aarch32.o
obj-y += mt_dbg_aarch32.o

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_mt_dbg_aarch32.o := -DDEBUG
CFLAGS_hw_watchpoint_aarch32.o := -DDEBUG
endif
