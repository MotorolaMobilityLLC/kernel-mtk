obj-y += lastpc.o

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_lastpc.o := -DDEBUG
endif

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
