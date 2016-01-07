ccflags-y += -I$(srctree)/drivers/misc/mediatek/mu3d/drv \
             -I$(srctree)/drivers/misc/mediatek/mu3d/hal \
             -I$(srctree)/drivers/misc/mediatek/mu3phy

ccflags-y += -DUSE_SSUSB_QMU

obj-y := mu3d_hal.o

mu3d_hal-y := mu3d_hal_osal.o \
              mu3d_hal_qmu_drv.o \
              mu3d_hal_usb_drv.o \
              mu3d_hal_phy.o
