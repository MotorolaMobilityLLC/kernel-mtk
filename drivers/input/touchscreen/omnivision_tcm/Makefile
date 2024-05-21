#
# Makefile for the Omnivision TCM touchscreen driver.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_spi.o
#obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_i2c.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_core_module.o
omnivision_tcm_core_module-objs := omnivision_tcm_core.o omnivision_tcm_touch.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_zeroflash.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_reflash.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_device.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_testing.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_csv_parser.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_recovery.o
obj-$(CONFIG_TOUCHSCREEN_OMNIVISION_TCM) += omnivision_tcm_diagnostics.o

