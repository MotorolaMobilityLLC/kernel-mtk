ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/spm/common/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/spm/$(MTK_PLATFORM)/

# Include cpu dvfs header cpufreq_api.h and thermal header mtk_thermal.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/
# Include mtk_ram_console.h for aee_xxx api
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
# Include uart header mtk_uart_api.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/uart/include/
# Include teei header teei_client_main.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/teei/V1.0/tz_driver/include/
# Include pwm header ddp_pwm.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
# Include mcdi header mtk_mcdi_governor.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include/
# Include clkbuf header mtk_clkbuf_ctl.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1/

obj-y += mtk_idle.o
obj-y += mtk_idle_select.o
obj-y += mtk_idle_internal.o
obj-y += mtk_idle_profile.o
obj-y += mtk_idle_twam.o
obj-y += mtk_dpidle.o
obj-y += mtk_sodi.o
obj-y += mtk_sodi3.o
obj-y += mtk_spm_resource_req.o
obj-y += mtk_sleep_init.o
obj-y += mtk_sleep.o
obj-y += mtk_spm_sleep.o