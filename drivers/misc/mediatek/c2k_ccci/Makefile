
#ifeq ($(CONFIG_MTK_C2K_SUPPORT), yes)

#ifeq ($(EVDO_VIA_HAVE_VOLT_PROTECTION), yes)#if we have voltage protection in design
#ccflags-y += -DOEM_HAVE_VOLT_PROTECTION
#endif

#ccflags-y += -DWAKE_HOST_BY_SYNC

#obj-y := core.o power.o oem.o sync.o
#obj-y	+= cbp_sdio.o modem_sdio.o
#obj-y += f_rawbulk.o rawbulk.o rawbulk_transfer.o

#else

#obj- :=dummy.o

#endif
ifneq ($(CONFIG_MTK_ECCCI_C2K), y)
ifeq ($(CONFIG_MTK_C2K_SUPPORT), y)

ifeq ($(EVDO_VIA_HAVE_VOLT_PROTECTION), yes) #if we have voltage protection in design
ccflags-y += -DOEM_HAVE_VOLT_PROTECTION
endif

ccflags-y += -I$(srctree)/drivers/misc/mediatek/ccmni/
ccflags-y += -DWAKE_HOST_BY_SYNC

obj-$(CONFIG_MTK_C2K_SUPPORT) += c2k_sdio.o

c2k_sdio-y := core.o power.o c2k_hw.o sync.o
c2k_sdio-y += cbp_sdio.o modem_sdio.o
c2k_sdio-y += f_rawbulk.o rawbulk.o rawbulk_transfer.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

else

obj- :=dummy.o

endif

else

obj- :=dummy.o

endif
