
ifeq ($(BUILD_SMT), YES)
ccflags-y +=-DSMT_MODE
endif

#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/alsps/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include

obj-y	:=  stk3a5x.o

