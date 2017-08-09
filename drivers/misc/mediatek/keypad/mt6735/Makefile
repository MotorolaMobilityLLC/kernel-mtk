ifeq ($(CONFIG_MTK_TC1_FM_AT_SUSPEND),y)
ifeq ($(CONFIG_MT_SND_SOC_V3),y)
	EXTRA_CFLAGS += -I$(srctree)/sound/soc/mediatek/mt_soc_audio_v3
endif
endif
obj-$(CONFIG_MTK_KEYPAD) := hal_kpd.o mtk_kpd_bkl.o
