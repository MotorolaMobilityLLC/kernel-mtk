ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
	ifeq ($(CONFIG_ARM64), y)
		# armv8
		ifndef CONFIG_TRUSTONIC_TEE_VERSION
			obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += 302a/
		else
			obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += $(subst ",,$(CONFIG_TRUSTONIC_TEE_VERSION))/
		endif
	else
		# armv7
		obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
	endif
endif
