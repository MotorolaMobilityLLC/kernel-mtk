#
# Makefile for misc devices that really don't fit anywhere else.
#
include $(srctree)/drivers/misc/mediatek/Makefile.custom

#obj-y += sensordriver/sensordrive_lens.o

ifeq ($(CONFIG_ARCH_MT8127),y)
		obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)      += mt8127/fm50af/
else
	ifeq ($(CONFIG_ARCH_MT6580),y)
		obj-$(CONFIG_MTK_LENS_DW9814AF_SUPPORT)      += mt6580/dw9814af/
		obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)      += mt6580/dw9714af/
	else
		ifeq ($(CONFIG_ARCH_MT6592),y)
		obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT) +=  mt6592/dummy_lens/
		obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)    +=  mt6592/fm50af/
		obj-$(CONFIG_MTK_LENS_GAF001AF_SUPPORT)  +=  mt6592/gaf001af/
		obj-$(CONFIG_MTK_LENS_GAF002AF_SUPPORT)  +=  mt6592/gaf002af/
		obj-$(CONFIG_MTK_LENS_GAF008AF_SUPPORT)  +=  mt6592/gaf008af/
		obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)  +=  mt6592/dw9714af/
		obj-$(CONFIG_MTK_LENS_DW9714A_SUPPORT)   +=  mt6592/dw9714a/
		obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)  +=  mt6592/dw9718af/
		obj-$(CONFIG_MTK_LENS_AK7345AF_SUPPORT)  +=  mt6592/ak7345af/
		obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += mt6592/lc898122af/
		obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += mt6592/sensordrive/
		else
			ifeq ($(CONFIG_ARCH_MT6582),y)
			obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT) +=  mt6582/dummy_lens/
			obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)    +=  mt6582/fm50af/
			obj-$(CONFIG_MTK_LENS_GAF001AF_SUPPORT)  +=  mt6582/gaf001af/
			obj-$(CONFIG_MTK_LENS_GAF002AF_SUPPORT)  +=  mt6582/gaf002af/
			obj-$(CONFIG_MTK_LENS_GAF008AF_SUPPORT)  +=  mt6582/gaf008af/
			obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)  +=  mt6582/dw9714af/
			obj-$(CONFIG_MTK_LENS_DW9714A_SUPPORT)   +=  mt6582/dw9714a/
			obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)  +=  mt6582/dw9718af/
			else
				ifeq ($(CONFIG_ARCH_MT6752),y)
				obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT) += mt6752/dummy_lens/
				obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT) += mt6752/ad5820af/
				obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT) += mt6752/ad5823/
				obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT) += mt6752/ad5823af/
				obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT) += mt6752/bu6424af/
				obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT) += mt6752/bu6429af/
				obj-$(CONFIG_MTK_LENS_BU64745GWZAF_SUPPORT) += mt6752/bu64745gwzaf/
				obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT) += mt6752/dw9714af/
				obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT) += mt6752/dw9718af/
				obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT) += mt6752/fm50af/
				obj-$(CONFIG_MTK_LENS_MT9P017AF_SUPPORT) += mt6752/mt9p017af/
				obj-$(CONFIG_MTK_LENS_OV8825AF_SUPPORT) += mt6752/ov8825af/
				obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += mt6752/sensordrive/
				obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += mt6752/lc898122af/
				obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT) += mt6752/lc898212af/
				else
					ifneq (,$(filter y,$(CONFIG_ARCH_MT6735) $(CONFIG_ARCH_MT6735M) $(CONFIG_ARCH_MT6753)))
					obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT)   += mt6735/dummy_lens/
					obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT)    += mt6735/ad5820af/
					obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += mt6735/ad5823/
					obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += mt6735/ad5823af/
					obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT)    += mt6735/bu6424af/
					obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT)    += mt6735/bu6429af/
					obj-$(CONFIG_MTK_LENS_BU64745GWZAF_SUPPORT)+= mt6735/bu64745gwzaf/
					obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)    += mt6735/dw9714af/
					obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)    += mt6735/dw9718af/
					obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)      += mt6735/fm50af/
					obj-$(CONFIG_MTK_LENS_MT9P017AF_SUPPORT)   += mt6735/mt9p017af/
					obj-$(CONFIG_MTK_LENS_OV8825AF_SUPPORT)    += mt6735/ov8825af/
					obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += mt6735/sensordrive/
					obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT)  += mt6735/lc898122af/
					obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT)  += mt6735/lc898212af/
					obj-$(CONFIG_MTK_LENS_AK7345AF_SUPPORT)    += mt6735/ak7345af/
					else
						ifeq ($(CONFIG_ARCH_MT6795),y)
						obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT)   += mt6795/dummy_lens/
						obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT)    += mt6795/ad5820af/
						obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += mt6795/ad5823/
						obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += mt6795/ad5823af/
						obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT)    += mt6795/bu6424af/
						obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT)    += mt6795/bu6429af/
						obj-$(CONFIG_MTK_LENS_BU64745GWZAF_SUPPORT)+= mt6795/bu64745gwzaf/
						obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)    += mt6795/dw9714af/
						obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)    += mt6795/dw9718af/
						obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)      += mt6795/fm50af/
						obj-$(CONFIG_MTK_LENS_MT9P017AF_SUPPORT)   += mt6795/mt9p017af/
						obj-$(CONFIG_MTK_LENS_OV8825AF_SUPPORT)    += mt6795/ov8825af/
						obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += mt6795/sensordrive/
						obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += mt6795/lc898122af/
						obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT)    += mt6795/lc898212af/
						obj-$(CONFIG_MTK_LENS_AK7345AF_SUPPORT)    += mt6795/ak7345af/
						else
							ifeq ($(CONFIG_ARCH_MT6755),y)
							obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT) += mt6755/dummy_lens/
							obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT) += mt6755/ad5820af/
							obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT) += mt6755/ad5823/
							obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT) += mt6755/ad5823af/
							obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT) += mt6755/bu6424af/
							obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT) += mt6755/bu6429af/
							obj-$(CONFIG_MTK_LENS_BU64745GWZAF_SUPPORT) += mt6755/bu64745gwzaf/
							obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT) += mt6755/dw9714af/
							obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT) += mt6755/dw9718af/
							obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT) += mt6755/fm50af/
							obj-$(CONFIG_MTK_LENS_MT9P017AF_SUPPORT) += mt6755/mt9p017af/
							obj-$(CONFIG_MTK_LENS_OV8825AF_SUPPORT) += mt6755/ov8825af/
							obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += mt6755/sensordrive/
							obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += mt6755/lc898122af/
							obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT) += mt6755/lc898212af/
							else
							obj-$(CONFIG_MTK_LENS_DUMMYLENS_SUPPORT)   += common/dummy_lens/
							obj-$(CONFIG_MTK_LENS_AD5820AF_SUPPORT)    += common/ad5820af/
							obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += common/ad5823/
							obj-$(CONFIG_MTK_LENS_AD5823AF_SUPPORT)    += common/ad5823af/
							obj-$(CONFIG_MTK_LENS_BU6424AF_SUPPORT)    += common/bu6424af/
							obj-$(CONFIG_MTK_LENS_BU6429AF_SUPPORT)    += common/bu6429af/
							obj-$(CONFIG_MTK_LENS_BU64745GWZAF_SUPPORT)+= common/bu64745gwzaf/
							obj-$(CONFIG_MTK_LENS_DW9714AF_SUPPORT)    += common/dw9714af/
							obj-$(CONFIG_MTK_LENS_DW9718AF_SUPPORT)    += common/dw9718af/
							obj-$(CONFIG_MTK_LENS_FM50AF_SUPPORT)      += common/fm50af/
							obj-$(CONFIG_MTK_LENS_MT9P017AF_SUPPORT)   += common/mt9p017af/
							obj-$(CONFIG_MTK_LENS_OV8825AF_SUPPORT)    += common/ov8825af/
							obj-$(CONFIG_MTK_LENS_SENSORDRIVE_SUPPORT) += common/sensordrive/
							obj-$(CONFIG_MTK_LENS_LC898122AF_SUPPORT) += common/lc898122af/
							obj-$(CONFIG_MTK_LENS_LC898212AF_SUPPORT)    += common/lc898212af/
							obj-$(CONFIG_MTK_LENS_AK7345AF_SUPPORT)    += common/ak7345af/
							obj-$(CONFIG_MTK_LENS_WV511AAF_SUPPORT)    += mt8163/wv511aaf/
							endif
						endif
					endif
				endif
			endif
		endif
	endif
endif



