ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/linux/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/core/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common \
	-I$(srctree)/drivers/misc/mediatek/connectivity/common_detect



ifeq ($(CONFIG_MTK_COMBO),y)
PLAT := alps
obj-y	+= wmt_plat_$(PLAT).o
obj-y	+= wmt_plat_stub.o
obj-y	+= mtk_wcn_cmb_hw.o


endif
