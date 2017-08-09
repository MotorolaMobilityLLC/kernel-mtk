#ccflags-y += -Werror

ifeq ($(CONFIG_MTK_GPU_R5P0),y)
$(info GPU BUILD r5p0)
obj-y += mali-EAC/drivers/
else
$(info GPU BUILD r7p0)
obj-y += mali-r7p0/drivers/
endif
