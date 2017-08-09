PLATFORM_UTGARD = mt6580
PLATFORM_MIDGARD = mt6735

PLATFORM_NAME = $(subst ",,$(CONFIG_MTK_PLATFORM))

ifneq (, $(findstring $(PLATFORM_NAME), $(PLATFORM_UTGARD)))
    $(info *GPU in $(PLATFORM_NAME) is: UTGARD-GPU)
    obj-y += mali_utgard/
else
    $(info *GPU in $(PLATFORM_NAME) is: MIDGARD-GPU)
    obj-y += mali_midgard/
endif