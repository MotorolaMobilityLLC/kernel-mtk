obj-$(CONFIG_MTK_MEMCFG) += mtk_memcfg.o
obj-y += mtk_meminfo.o

MTK_PLATFORM_DIR=$(srctree)/drivers/misc/mediatek/mem/$(subst ",,$(CONFIG_MTK_PLATFORM))
mtkplatform_exist=$(shell if [ -d ${MTK_PLATFORM_DIR} ]; then echo "y"; else echo "n"; fi;)
ifeq ($(mtkplatform_exist), y)
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
