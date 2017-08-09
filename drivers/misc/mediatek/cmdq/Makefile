# common driver code for CMDQ
# 72/82/92 does not use this common code

# use common driver
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6795" "mt6752" "mt6735" "mt6580" "mt6755" "mt6797"))
  # v2 driver
  obj-y += v2/
else
  obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
# EOF