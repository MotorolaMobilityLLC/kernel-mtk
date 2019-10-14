
# FocalTech 2-1: Please edit ${KERNEL_SRC}/drivers/input/fingerprint/Kconfig to add FOCALTECH_FINGERPRINT option.
config FOCALTECH_FINGERPRINT
    tristate "FocalTech Fingerprint"
    default y
    help
      If you say Y to this option, support will be included for
      the FocalTech's fingerprint sensor. This driver supports
      both REE and TEE.

      This driver can also be built as a module. If so, the module
      will be called 'focaltech_fp'.


# FocalTech 2-2: Please edit ${KERNEL_SRC}/drivers/input/fingerprint/Makefile to add FOCALTECH_FINGERPRINT option.
obj-$(CONFIG_FOCALTECH_FINGERPRINT) += focaltech/

