#######################################
# Specify source files
#######################################

# BUILD-IN
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/gcpu/$(MTK_PLATFORM)),)
obj-y += $(MTK_PLATFORM)/
else
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += $(subst ",,common)/
obj-$(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) += $(subst ",,mt8xxx)/
endif
