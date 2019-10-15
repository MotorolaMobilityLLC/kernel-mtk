#
# Makefile for the fingerprint drivers.
#
subdir-ccflags-$(CONFIG_TINNO_PRODUCT_INFO) += -I$(srctree)/tinno/common_features/dev_info
TARGET_NAME := \
    $(strip $(subst ro.target =,,$(shell cat $(PWD)/build.ini |grep ro.target)))

fingerprint_supprot := \
    $(strip $(subst TINNO_FINGERPRINT_SUPPORT =,,$(shell cat $(PWD)/build.ini |grep TINNO_FINGERPRINT_SUPPORT)))

$(warning ---------------fingerprint_supprot--$(fingerprint_supprot)--------------)
ifneq ($(strip $(fingerprint_supprot)),)
    # subdir-ccflags-y += $(addprefix -D, $(shell echo $(fingerprint_supprot) | tr a-z A-Z))
    subdir-ccflags-y += -DTINNO_FINGERPRINT_SUPPORT
    fingerprint_drv := $(addsuffix /, $(shell echo $(fingerprint_supprot)) )
    obj-y += $(fingerprint_drv)
endif

# fp_drv
obj-y += fp_drv/




