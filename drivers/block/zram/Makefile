zram-y	:=	zcomp_lzo.o zcomp.o zram_drv.o

zram-$(CONFIG_ZRAM_LZ4_COMPRESS) += zcomp_lz4.o

zram-$(CONFIG_ZRAM_LZ4K_COMPRESS) += zcomp_lz4k.o

obj-$(CONFIG_ZRAM)	+=	zram.o

# zram accelerator implementation
obj-$(CONFIG_HWZRAM_IMPL)      += hwzram_impl.o

# vendors for zram accelerator
obj-$(CONFIG_HWZRAM_IMPL)      += vendor/

# hw zram driver
obj-$(CONFIG_HWZRAM_DRV)       += hwzram_drv.o
