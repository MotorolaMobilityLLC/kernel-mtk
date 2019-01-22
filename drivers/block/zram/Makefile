zram-y	:=	zcomp_lzo.o zcomp.o zram_drv.o

zram-$(CONFIG_ZRAM_LZ4_COMPRESS) += zcomp_lz4.o

obj-$(CONFIG_ZRAM)	+=	zram.o

# ZRAM accelerator implementation
obj-$(CONFIG_HWZRAM_IMPL)      += hwzram_impl.o
obj-$(CONFIG_HWZRAM_IMPL_TEST) += hwzram_impl_test.o

# Vendors for ZRAM accelerator
obj-$(CONFIG_HWZRAM_IMPL)      += vendor/

# HW ZRAM
obj-$(CONFIG_HWZRAM_DRV)       += hwzram_drv.o
