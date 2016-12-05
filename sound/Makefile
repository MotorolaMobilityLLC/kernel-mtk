# Makefile for the Linux sound card driver
#

subdir-ccflags-y += -Werror -Wno-error=maybe-uninitialized

obj-$(CONFIG_SOUND) += soundcore.o
obj-$(CONFIG_SOUND_PRIME) += sound_firmware.o
obj-$(CONFIG_SOUND_PRIME) += oss/
obj-$(CONFIG_DMASOUND) += oss/
obj-$(CONFIG_SND) += core/ i2c/ drivers/ isa/ pci/ ppc/ arm/ sh/ synth/ usb/ \
	firewire/ sparc/ spi/ parisc/ pcmcia/ mips/ soc/ atmel/
obj-$(CONFIG_SND_AOA) += aoa/

# This one must be compilable even if sound is configured out
obj-$(CONFIG_AC97_BUS) += ac97_bus.o
obj-$(CONFIG_EXTPA_AW87318_AW87319_SWITCH) += lct_aw8731x/
ifeq ($(CONFIG_SND),y)
  obj-y += last.o
endif

soundcore-objs  := sound_core.o
