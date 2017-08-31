ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include

obj-y += typec.o typec-ioctl.o typec-test.o typec-pltfrm.o
obj-y += pd-policy.o pd-policy-cust.o pd-protocol.o
obj-$(CONFIG_DUAL_ROLE_USB_INTF)	+= pd-dual_role.o
obj-y += usb_switch.o

#_MT6336_E1 = y

ifdef _MT6336_E1
	ccflags-y += -DMT6336_E1
else
	ccflags-y += -DMT6336_E2
endif