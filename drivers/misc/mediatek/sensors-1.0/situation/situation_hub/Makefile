ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/situation
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/

# moto add
ifeq ($(CONFIG_MOTO_HUB),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/cm4
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/include
endif

obj-$(CONFIG_MTK_SENSOR_SUPPORT)	+= situationhub.o
situationhub-y := situation_hub.o
situationhub-$(CONFIG_MTK_INPKHUB) += inpocket/inpocket.o
situationhub-$(CONFIG_MTK_STATHUB) += stationary/stationary.o
situationhub-$(CONFIG_MTK_WAKEHUB) += wake_gesture/wake_gesture.o
situationhub-$(CONFIG_MTK_GLGHUB) += glance_gesture/glance_gesture.o
situationhub-$(CONFIG_MTK_PICKUPHUB) += pickup_gesture/pickup_gesture.o
situationhub-$(CONFIG_MTK_ANSWER_CALL_HUB) += answercall/ancallhub.o
situationhub-$(CONFIG_MTK_DEVICE_ORIENTATION_HUB) += device_orientation/device_orientation.o
situationhub-$(CONFIG_MTK_MOTION_DETECT_HUB) += motion_detect/motion_detect.o
situationhub-$(CONFIG_MTK_TILTDETECTHUB) += tilt_detector/tiltdetecthub.o
situationhub-$(CONFIG_MTK_FLAT_HUB) += flat/flat.o
situationhub-$(CONFIG_MTK_SAR_HUB) += sar/sarhub.o sar/sar_factory.o

#moto add
ifeq ($(CONFIG_MOTO_HUB),y)
situationhub-$(CONFIG_MOTO_CHOPCHOP) += mot_hub/moto_chopchop/moto_chopchop.o
situationhub-$(CONFIG_MOTO_CAMGEST) += mot_hub/moto_camgest/moto_camgest.o
situationhub-$(CONFIG_MOTO_FLATDOWN) += mot_hub/moto_flatdown/moto_flatdown.o
situationhub-$(CONFIG_MOTO_FLATUP) += mot_hub/moto_flatup/moto_flatup.o
situationhub-$(CONFIG_MOTO_FTM) += mot_hub/moto_ftm/moto_ftm.o
situationhub-$(CONFIG_MOTO_LTS) += mot_hub/moto_lts/moto_lts.o
situationhub-$(CONFIG_MOTO_LTV) += mot_hub/moto_ltv/moto_ltv.o
situationhub-$(CONFIG_MOTO_GLANCE) += mot_hub/moto_glance/moto_glance.o
situationhub-$(CONFIG_MOTO_OFFBODY) += mot_hub/moto_offbody/moto_offbody.o
situationhub-$(CONFIG_MOTO_STOWED) += mot_hub/moto_stowed/moto_stowed.o
endif