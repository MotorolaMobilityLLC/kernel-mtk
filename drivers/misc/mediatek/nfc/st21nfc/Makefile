#
# Makefile for nfc devices
#

ifneq ($(CONFIG_NFC_CHIP_SUPPORT),)
obj-y  += st21nfc.o
endif

ccflags-$(CONFIG_NFC_DEBUG) := -DDEBUG
