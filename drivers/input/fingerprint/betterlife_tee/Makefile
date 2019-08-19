#ccflags-y += -DCOMPATIBLE
ccflags-y += -DCOMPATIBLE_IN_HAL
#ccflags-y += -DCONFIG_MTK_CLK
#ccflags-y += -DNEED_OPT_POWER_ON1V8
#ccflags-y += -DNEED_OPT_POWER_ON2V8
#ccflags-y += -DREGISTER_DEVICE
#for mt6797  Rx_DMA address range error in 4G mode,dma:0
#ccflags-y += -DUSE_SPI1_4GB_TEST
#Only MTK && >= Android M BF_PINCTL to be set
#ccflags-y += -DBF_PINCTL


#PLATFORM_MTK_L PLATFORM_MTK PLATFORM_MT6739 PLATFORM_SPRD PLATFORM_QCOM PLATFORM_RK PLATFORM_MT676x
BL_PLATFORM := PLATFORM_MT6739

#TEE_TK TEE_BEANPOD REE other TEE_PLATFORM add #
TEE_PLATFORM := TEE_TK

ifeq ($(strip $(TEE_PLATFORM)), TEE_TK)
$(info "************* trustkernel tee ***************")
ccflags-y += -DTEE_TK
ccflags-y += -DCONFIG_MTK_CLK
endif

ifeq ($(strip $(TEE_PLATFORM)), TEE_BEANPOD)
$(info "**************** beanpod tee ****************")
ccflags-y += -DTEE_BEANPOD
ccflags-y += -DCONFIG_MTK_CLK
endif

ifeq ($(strip $(TEE_PLATFORM)), REE)
$(info "********************** ree ******************")
ccflags-y += -DBF_REE
ccflags-y += -DCOMPATIBLE
endif

ifneq (,$(filter $(strip $(BL_PLATFORM)),PLATFORM_MTK_L PLATFORM_MTK PLATFORM_MT6739 PLATFORM_MT676x))

ifeq ($(strip $(BL_PLATFORM)), PLATFORM_MTK_L)
BL_PLATFORM := PLATFORM_MTK
ccflags-y += -DMTK_ANDROID_L
ccflags-y += -DREGISTER_DEVICE
include $(srctree)/drivers/misc/mediatek/Makefile.custom
else
ccflags-y += -DBF_PINCTL
endif

ifndef CONFIG_MTK_PLATFORM
# most mediatek platform will define CONFIG_MTK_PLATFORM, if your platform is mediatek but not define CONFIG_MTK_PLATFORM, please ignore the next line error
$(error "************* platform is not mediatek, please set BL_PLATFORM as PLATFORM_SPRD or PLATFORM_QCOM *************")
else
$(warning "CONFIG_MTK_PLATFORM is $(CONFIG_MTK_PLATFORM)")
ifeq ($(CONFIG_MTK_PLATFORM), "mt6739")
BL_PLATFORM := PLATFORM_MT6739
endif
ifeq ($(CONFIG_MTK_PLATFORM), "mt6580")
ccflags-y += -DUSE_ENABLE_CLOCK
endif
ifneq (,$(filter "mt676%" "mt677%",$(CONFIG_MTK_PLATFORM)))
ifeq ($(CONFIG_MTK_PLATFORM), "mt6763")
BL_PLATFORM := PLATFORM_MTK 
else
BL_PLATFORM := PLATFORM_MT676x 
endif
endif
ifeq ($(CONFIG_MTK_PLATFORM), "mt6797")
#for mt6797  Rx_DMA address range error in 4G mode,dma:0
ifeq ($(strip $(TEE_PLATFORM)), REE)
ccflags-y += -DUSE_SPI1_4GB_TEST  
endif
endif
endif
endif

ifeq ($(strip $(BL_PLATFORM)), PLATFORM_MTK)
ccflags-y += -DPLATFORM_MTK
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat -Wno-unused
endif

ifneq (,$(filter $(BL_PLATFORM),PLATFORM_MT6739 PLATFORM_MT676x))
ifeq ($(strip $(BL_PLATFORM)), PLATFORM_MT6739)
ccflags-y += -DPLATFORM_MT6739
endif
ifeq ($(strip $(BL_PLATFORM)), PLATFORM_MT676x)
ccflags-y += -DPLATFORM_MT676x
endif

ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat -Wno-unused
endif

ifeq ($(strip $(BL_PLATFORM)), PLATFORM_SPRD)
ccflags-y += -DPLATFORM_SPRD
endif

ifeq ($(strip $(BL_PLATFORM)), PLATFORM_QCOM)
ccflags-y += -DPLATFORM_QCOM
endif

ifeq ($(strip $(BL_PLATFORM)), PLATFORM_RK)
ccflags-y += -DPLATFORM_RK
endif

# -w -Wno-strict-aliasing -Wno-error=unused-function -Wno-error=unused-label -Wno-warning=unused-function -Wno-warning=unused-label
ccflags-y += -Wno-unused-label -Wno-unused-variable
obj-y :=  bf_fp_platform.o bf_fp_spi.o
