
ifneq ($(MTK_ROOT_BUILD),)
    $(info "MTK_ROOT_BUILD secwidevinemdw")
    include $(MTK_ROOT_BUILD)/Makefile
else
    $(info "custom build secwidevinemdw")
endif

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
    $(info "Build widevine modular drm driver")
    $(info "MTK_PATH_PLATFORM=$(MTK_PATH_PLATFORM)")
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/trustzone/utils/tlutils
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
    obj-y += secwidevinemdw.o
endif
