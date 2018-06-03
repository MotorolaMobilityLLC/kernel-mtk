ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-$(CONFIG_MACH_MT6799) += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt6799/include/mach

obj-y += typec.o typec-ioctl.o typec-pltfrm.o
obj-y += pd-policy.o pd-policy-cust.o pd-protocol.o
obj-$(CONFIG_DUAL_ROLE_USB_INTF)	+= pd-dual_role.o
obj-y += usb_switch.o
obj-$(CONFIG_RT7207_ADAPTER) += mtk_direct_charge_vdm.o

#_MT6336_E1 = y

ifdef _MT6336_E1
	ccflags-y += -DMT6336_E1
else
	ccflags-y += -DMT6336_E2
endif
