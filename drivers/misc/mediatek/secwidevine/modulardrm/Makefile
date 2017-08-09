
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
    ccflags-y += -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/include \
                 -I$(srctree)/drivers/misc/mediatek/gud/$(MTK_PLATFORM)/gud/MobiCoreKernelApi/public
    obj-y += secwidevinemdw.o
endif
