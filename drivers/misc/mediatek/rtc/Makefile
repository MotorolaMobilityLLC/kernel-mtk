MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

#ifneq (,$(filter $(strip $(CONFIG_MTK_PLATFORM)), mt6735 mt6735m mt6753 mt6580 mt6572 mt6582 mt6592 mt6752 mt6795 mt6755))

#set IP folder name
ifneq (,$(filter y,$(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753)))
    MDI_IP_FOLDER:=mt6328
else
    ifneq (,$(filter y,$(CONFIG_ARCH_MT6795)))
	    MDI_IP_FOLDER:=mt6331
    else
        ifneq (,$(filter y,$(CONFIG_ARCH_MT6572) $(CONFIG_ARCH_MT6582) $(CONFIG_ARCH_MT6592)))
	        MDI_IP_FOLDER:=mt6323
        else
            ifneq (,$(filter y,$(CONFIG_ARCH_MT6752)))
	            MDI_IP_FOLDER:=mt6325
            else
              ifneq (,$(filter y,$(CONFIG_ARCH_MT6580)))
	              MDI_IP_FOLDER:=mt6350
              else
                ifneq (,$(filter y,$(CONFIG_ARCH_MT6755)))
	                MDI_IP_FOLDER:=mt6351
	              else
	                ifneq (,$(filter y,$(CONFIG_ARCH_MT8173)))
	                MDI_IP_FOLDER:=mt6391
	                endif
                endif
              endif
            endif
        endif
    endif
endif

MDP_COMMON_FOLDER:= ./

ifneq (,$(filter $(strip $(MTK_PLATFORM)), mt8163))
MDI_IP_FOLDER:=mt8163
endif

#Top level common code
obj-$(CONFIG_MTK_RTC) += mtk_rtc_common.o mtk_rtc_hal_common.o

MTK_MDI_IP_SRC:= $(MDI_IP_FOLDER)/

#IP dependent part
obj-y += \
    $(MTK_MDI_IP_SRC)

