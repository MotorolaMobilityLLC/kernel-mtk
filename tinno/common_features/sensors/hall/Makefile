
obj-$(CONFIG_CUSTOM_KERNEL_HALL)	+= hall_device.o

WIKO_UNIFY_DEF := $(strip $(subst ro.wiko.type =,,$(shell cat $(PWD)/build.ini |grep ^ro.wiko.type)))
ifeq ($(strip $(WIKO_UNIFY_DEF)),unify)
ccflags-y += -DCONFIG_WIKO_UNIFY
$(warning ---add marco CONFIG_WIKO_UNIFY---)
endif

