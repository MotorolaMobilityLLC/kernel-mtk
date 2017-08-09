$(info IRTX: PWM+IRTX:$(CONFIG_MTK_IRTX_SUPPORT))
$(info IRTX: PWM ONLY:$(CONFIG_MTK_IRTX_PWM_SUPPORT))

ifeq ($(CONFIG_MTK_IRTX_SUPPORT), y)
obj-y := mt_irtx.o
else
ifeq ($(CONFIG_MTK_IRTX_PWM_SUPPORT), y)
obj-y := mt_irtx_pwm.o
else
obj- := dummy.o
endif
endif

