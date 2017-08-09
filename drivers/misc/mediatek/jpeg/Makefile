# drivers/image/jpeg/Makefile

# use common driver
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6735" "mt6753" "mt6755" "mt6595" "mt6752" "mt6795" "mt6797" "mt8173"))
  # new chip use v2 driver
  obj-y += v2/
else
  # legacy chip use v1 driver
  ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6582" "mt6592"))
    obj-y += v1/
  else
     ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt8163"))
       obj-y += mt8163/
     else
      ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt8127"))
       obj-y += mt8127/
      else
       ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt7623"))
       obj-y += mt7623/
       endif
      endif
     endif
  endif
endif
# EOF
