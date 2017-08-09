#######################################
# Specify source files
#######################################

# BUILD-IN
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += common/    ### For MT6xxx
obj-$(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) += mt8xxx/ ### For MT8xxx
ifeq ($(CONFIG_HAVE_MTK_IN_HOUSE_TEE_SUPPORT),y)
obj-$(CONFIG_TRUSTY) += mt8xxx/ ### For MT8xxx
endif
