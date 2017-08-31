# Makefile for ARM cc710ree Linux device driver

MODULE_NAME = cc710ree

include $(srctree)/drivers/misc/mediatek/dxcc/cc_driver/cc710ree/proj.cfg

obj-y := $(MODULE_NAME).o
$(MODULE_NAME)-objs := ssi_driver.o ssi_sysfs.o ssi_buffer_mgr.o ssi_request_mgr.o ssi_cipher.o ssi_hash.o ssi_aead.o ssi_ivgen.o ssi_sram_mgr.o ssi_pm.o ssi_pm_ext.o

#FIPS
ifeq ($(DX_CONFIG_SUPPORT_FIPS),1)
EXTRA_CFLAGS += -DSSI_SUPPORT_FIPS
$(MODULE_NAME)-objs += ssi_fips.o ssi_fips_ll.o ssi_fips_ext.o ssi_fips_local.o
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
EXTRA_CFLAGS += -DTRUSTONIC_FIPS_SUPPORT
EXTRA_CFLAGS += -I$(srctree)/drivers/misc/mediatek/gud/$(CONFIG_TRUSTONIC_TEE_VERSION)/gud/MobiCoreDriver/public
endif
endif

EXTRA_CFLAGS += -DDX_CC_REE #-DCOMP_IN_WQ
EXTRA_CFLAGS += -DDX_ROM_VERSION=$(DX_ROM_VERSION)
ifneq ($(DX_CC_SRAM_SIZE),)
EXTRA_CFLAGS += -DDX_CC_SRAM_SIZE=$(DX_CC_SRAM_SIZE)
endif
ifeq ($(DX_CONFIG_DISABLE_COHERENT_DMA_OPS),1)
EXTRA_CFLAGS += -DDISABLE_COHERENT_DMA_OPS
endif
ifeq ($(DX_FPGA_ENV),1)
EXTRA_CFLAGS += -DDX_FPGA_ENV
endif

ifeq ($(DX_CONFIG_TEST_48BIT_DMA_ADDR),1)
EXTRA_CFLAGS += -DDX_DMA_48BIT_SIM
endif

ifeq ($(DEBUG),1) # Remove optimization flags
EXTRA_CFLAGS += -DDX_DEBUG
define remove_o2
	CFLAGS_REMOVE_$(1) = -O2
endef
$(foreach obj,$(MODULE_NAME)-objs,$(eval $(call remove_o2,$(obj))))
EXTRA_CFLAGS += -DDEBUG -O1
endif

EXTRA_CFLAGS += -Wall

$(info $(if $(DEBUG),DEBUG=$(DEBUG)))
