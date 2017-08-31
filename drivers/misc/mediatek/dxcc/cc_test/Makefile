# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

MODULE_NAME = ssi_cc_ll_test

obj-y := cc_ll_test.o
#$(MODULE_NAME)-objs := cc_ll_test.o

EXTRA_CFLAGS = -DDX_CC_REE

ifeq ($(DEBUG),1)
  # Remove optimization flags
  CFLAGS_REMOVE_cc_ll_test.o = -O2
  EXTRA_CFLAGS += -DDEBUG -O1
endif

$(info $(if $(DEBUG),DEBUG=$(DEBUG)))
