obj-y += hw_watchpoint_aarch64.o
obj-y += mt_dbg_aarch64.o

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_mt_dbg_aarch64.o := -DDEBUG
CFLAGS_hw_watchpoint_aarch64.o := -DDEBUG
endif
